/*  ====================================================================================================================
    Copyright (C) 2020 Eaton
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    ====================================================================================================================
*/

#include "assets.h"
#include "impl/mibs.h"
#include "impl/nut/mapper.h"
#include "impl/nut/process.h"
#include "impl/ping.h"
#include "impl/uuid.h"
#include <fty/split.h>

namespace fty::job {

void Assets::run(const commands::assets::In& in, commands::assets::Out& out)
{
    if (!available(in.address)) {
        throw Error("Host is not available: {}", in.address.value());
    }

    m_params = in;
    // Workaround to check if snmp is available. Read mibs from asset
    if (m_params.protocol == "nut_snmp") {
        if (!m_params.port.hasValue()) {
            m_params.port = 161;
        }

        impl::MibsReader reader(m_params.address, uint16_t(m_params.port.value()));

        if (m_params.settings.credentialId.hasValue()) {
            if (auto res = reader.setCredentialId(m_params.settings.credentialId); !res) {
                throw Error(res.error());
            }
        } else if (m_params.settings.community.hasValue()) {
            if (auto res = reader.setCommunity(m_params.settings.community); !res) {
                throw Error(res.error());
            }
        } else {
            throw Error("Credential or community must be set");
        }

        if (auto mibs = reader.read(); !mibs) {
            throw Error(mibs.error());
        } else {
            if (!m_params.settings.mib.hasValue()) {
                m_params.settings.mib = *mibs->begin();
            }
        }
    }

    // Runs nut process
    impl::nut::Process proc(m_params.protocol);
    if (auto res = proc.init(m_params.address, uint16_t(m_params.port.value()))) {
        if (m_params.settings.credentialId.hasValue()) {
            if (auto set = proc.setCredentialId(m_params.settings.credentialId); !set) {
                throw Error(set.error());
            }
        } else if (m_params.settings.community.hasValue()) {
            if (auto set = proc.setCommunity(m_params.settings.community); !set) {
                throw Error(set.error());
            }
        }
        if (m_params.settings.username.hasValue() && m_params.settings.password.hasValue()) {
            proc.setCredential(m_params.settings.username, m_params.settings.password);
        }

        if (m_params.settings.timeout.hasValue()) {
            proc.setTimeout(m_params.settings.timeout);
        }

        if (m_params.settings.mib.hasValue()) {
            proc.setMib(m_params.settings.mib);
        }

        if (auto cnt = proc.run()) {
            parse(*cnt, out);
        } else {
            throw Error(cnt.error());
        }
    } else {
        throw Error(res.error());
    }
}

void Assets::parse(const std::string& cnt, commands::assets::Out& out)
{
    static std::regex rex("([a-z0-9\\.]+)\\s*:\\s+(.*)");

    std::map<std::string, std::string> tmpMap;

    log_debug(cnt.c_str());

    std::stringstream ss(cnt);
    for (std::string line; std::getline(ss, line);) {
        auto [key, value] = fty::split<std::string, std::string>(line, rex);
        tmpMap.emplace(key, value);
    }

    //Get the device type
    auto it = tmpMap.find("device.type");
    std::string deviceType = it != tmpMap.end() ? fty::convert<std::string>(it->second) : "";

    it = tmpMap.find("device.count");

    int dcount = it != tmpMap.end() ? fty::convert<int>(it->second) : 0;
    if (dcount > 1) { //daisy chain is always bigger than one
        // daisychain
        for (int i = 0; i < dcount; ++i) {
            auto& asset      = out.append();
            asset.subAddress = std::to_string(i + 1);
            asset.asset.type = "device";
            asset.asset.subtype = deviceType;

            std::string prefix = "device." + std::to_string(i + 1) + ".";
            for (const auto& p : tmpMap) {
                if (p.first.find(prefix) != 0) {
                    continue;
                }

                if (auto key = impl::nut::Mapper::mapKey(p.first.substr(prefix.size())); !key.empty()) {
                    addAssetVal(asset.asset, key, p.second);
                }
            }
            enrichAsset(asset);

        }
    } else {
        auto& asset      = out.append();
        asset.subAddress = "";
        asset.asset.type = "device";
        asset.asset.subtype = deviceType;

        for (const auto& p : tmpMap) {
            if (auto key = impl::nut::Mapper::mapKey(p.first); !key.empty()) {
                addAssetVal(asset.asset, key, p.second);
            }
        }
        enrichAsset(asset);
    }
}

void Assets::addAssetVal(commands::assets::Return::Asset& asset, const std::string& key, const std::string& val, bool readOnly)
{
    auto& ext = asset.ext.append();
    ext.append(key, val);
    ext.append("read_only", (readOnly ? "true" : "false"));
}

void Assets::enrichAsset(commands::assets::Return& asset)
{
    if(asset.asset.subtype.empty()) {
        auto type = asset.asset.ext.find([](const pack::StringMap& info) {
            return info.contains("device.type");
        });

        if (type != std::nullopt) {
            asset.asset.subtype = (*type)["device.type"];
        }
    }

    if (asset.asset.subtype == "pdu") {
        asset.asset.subtype = "epdu";
    }
    if (asset.asset.subtype == "ats") {
        asset.asset.subtype = "sts";
    }

    addAssetVal(asset.asset, "ip.1", m_params.address, false);
    addAssetVal(asset.asset, "endpoint.1.protocol", m_params.protocol, false);
    addAssetVal(asset.asset, "endpoint.1.port", std::to_string(m_params.port), false);
    addAssetVal(asset.asset, "endpoint.1.sub_address", asset.subAddress, false);
    addAssetVal(asset.asset, "endpoint.1.status.operating", "IN_SERVICE", false);
    addAssetVal(asset.asset, "endpoint.1.status.error_msg", "", false);

    auto manufacturer = asset.asset.ext.find([](const pack::StringMap& info) {
        return info.contains("manufacturer");
    });

    auto model = asset.asset.ext.find([](const pack::StringMap& info) {
        return info.contains("model");
    });

    auto serial = asset.asset.ext.find([](const pack::StringMap& info) {
        return info.contains("serial_no");
    });

    if (manufacturer != std::nullopt && model != std::nullopt && serial != std::nullopt) {
        addAssetVal(asset.asset, "uuid",
            fty::impl::generateUUID((*manufacturer)["manufacturer"], (*model)["model"], (*serial)["serial_no"]), false);
    } else {
        addAssetVal(asset.asset, "uuid", "", false);
    }

    //try to get realpower.nominal fro max_power
    auto realpower_nominal = asset.asset.ext.find([](const pack::StringMap& info) {
        return info.contains("realpower.nominal");
    });

    if( realpower_nominal !=  std::nullopt) {
        addAssetVal(asset.asset, "max_power", (*realpower_nominal)["realpower.nominal"] , false);
    } else {
        //try to get realpower.default.nominal fro max_power
        auto realpower_default_nominal = asset.asset.ext.find([](const pack::StringMap& info) {
            return info.contains("realpower.default.nominal");
        });

        if( realpower_default_nominal !=  std::nullopt) {
            addAssetVal(asset.asset, "max_power", (*realpower_default_nominal)["realpower.default.nominal"] , false);
        } 
    }

    auto serial = asset.asset.ext.find([](const pack::StringMap& info) {
        return info.contains("serial_no");
    });

    if (m_params.protocol == "nut_snmp") {
        if (m_params.settings.credentialId.hasValue()) {
            addAssetVal(asset.asset, "endpoint.1.nut_snmp.secw_credential_id", m_params.settings.credentialId, false);
        }
        if (m_params.settings.community.hasValue()) {
            addAssetVal(asset.asset, "endpoint.1.nut_snmp.community", m_params.settings.community, false);
        }
        if (m_params.settings.mib.hasValue()) {
            addAssetVal(asset.asset, "endpoint.1.nut_snmp.MIB", m_params.settings.mib, false);
        }
    }

    //epdu daisy_chaine attribut
    if(asset.asset.subtype == "epdu") {

        std::string daisyChain = "0";
        if(!asset.subAddress.empty()){
            daisyChain = asset.subAddress;
        }
        addAssetVal(asset.asset, "daisy_chain", daisyChain);
    }
}


} // namespace fty::job
