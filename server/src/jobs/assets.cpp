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
#include <fty/split.h>

namespace fty::job {

void Assets::run(const commands::assets::In& in, commands::assets::Out& out)
{
    if (!available(in.address)) {
        throw Error("Host is not available: {}", in.address.value());
    }

    m_params = in;
    // Workaround to check if snmp is available. Read mibs from asset
    if (m_params.protocol == "NUT_SNMP") {
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
        } else {
            throw Error("Credentials or community is not set");
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

    auto it = tmpMap.find("device.count");

    int dcount = it != tmpMap.end() ? fty::convert<int>(it->second) : 0;
    if (dcount) {
        // daisychain
        for (int i = 0; i < dcount; ++i) {
            auto& asset      = out.append();
            asset.subAddress = std::to_string(i + 1);
            asset.asset.type = "device";

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

        for (const auto& p : tmpMap) {
            if (auto key = impl::nut::Mapper::mapKey(p.first); !key.empty()) {
                addAssetVal(asset.asset, key, p.second);
            }
        }
        enrichAsset(asset);
    }
}

void Assets::addAssetVal(commands::assets::Return::Asset& asset, const std::string& key, const std::string& val)
{
    auto& ext = asset.ext.append();
    ext.append(key, val);
    ext.append("read_only", "true");
}

void Assets::enrichAsset(commands::assets::Return& asset)
{
    auto type = asset.asset.ext.find([](const pack::StringMap& info) {
        return info.contains("device.type");
    });
    if (type != std::nullopt) {
        asset.asset.subtype = (*type)["device.type"];
    }
    if (asset.asset.subtype == "pdu") {
        asset.asset.subtype = "epdu";
    }
    if (asset.asset.subtype == "ats") {
        asset.asset.subtype = "sts";
    }

    addAssetVal(asset.asset, "ip.1", m_params.address);
    addAssetVal(asset.asset, "endpoint.1.protocol", m_params.protocol);
    addAssetVal(asset.asset, "endpoint.1.port", std::to_string(m_params.port));
    addAssetVal(asset.asset, "endpoint.1.sub_address", asset.subAddress);

    if (m_params.protocol == "NUT_SNMP") {
        if (m_params.settings.credentialId.hasValue()) {
            addAssetVal(asset.asset, "endpoint.1.NUT_SNMP.secw_credential_id", m_params.settings.credentialId);
        }
        if (m_params.settings.community.hasValue()) {
            addAssetVal(asset.asset, "endpoint.1.NUT_SNMP.community", m_params.settings.community);
        }
        if (m_params.settings.mib.hasValue()) {
            addAssetVal(asset.asset, "endpoint.1.NUT_SNMP.MIB", m_params.settings.mib);
        }
    }

    auto status = asset.asset.ext.find([](const pack::StringMap& info) {
        return info.contains("status.ups");
    });
    if (status != std::nullopt) {
        auto sts = fty::split((*status)["status.ups"], " ");
        std::vector<std::string> st;
        for(const auto& it: sts) {
            if (it == "OL") {
                st.push_back("ONLINE");
            } else if (it == "OB"){
                st.push_back("ONBATTERY");
            } else if (it == "BOOST"){
                st.push_back("ONBOOST");
            } else if (it == "OFF"){
                st.push_back("SLEEPING");
            } else if (it == "BYPASS"){
                st.push_back("ONBYPASS");
            } else if (it == "TRIM"){
                st.push_back("ONBUCK");
            }
        }
        addAssetVal(asset.asset, "endpoint.1.status.operating", implode(st, "|"));
    } else {
        addAssetVal(asset.asset, "endpoint.1.status.operating", "UNKNOWN");
    }
}


} // namespace fty::job
