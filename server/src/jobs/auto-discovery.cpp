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

#include "auto-discovery.h"
#include "protocols.h"
#include "assets.h"
#include <asset/asset-manager.h>
#include <fty_asset_dto.h>

namespace fty::job {

Expected<void> AutoDiscovery::updateExt(const commands::assets::Ext &ext_in, fty::asset::create::Ext &ext_out)
{
    // Construct and update output ext attributes according input ext attributes
    // [{ "key": "val", "read_only": "true" },...] -> {
    //                                                  "key:" {
    //                                                    "value": "val",
    //                                                    "readOnly: true,
    //                                                    "update": true
    //                                                  },
    //                                                  ...
    //                                                }
    for (auto it = ext_in.begin(); it != ext_in.end(); it++) {
        fty::asset::create::MapExtObj newMapExtObj;
        std::string key;
        // for each attribute
        for (auto it_attr = it->begin(); it_attr != it->end(); it_attr++) {
            if (it_attr->first == "read_only") {
                newMapExtObj.readOnly = it_attr->second == "true" ? true : false;
            }
            else {
                // It is the name of the key and its associated value
                key = it_attr->first;
                newMapExtObj.value = it_attr->second;
            }
        }
        if (!key.empty()) {
            ext_out.append(key, newMapExtObj);
        }
    }
    return {};
}

Expected<void> AutoDiscovery::updateHostName(const std::string& address, fty::asset::create::Ext &ext)
{
    if (!address.empty()) {
        // Set host name
        struct sockaddr_in sa_in;
        struct sockaddr* sa  = reinterpret_cast<sockaddr*>(&sa_in);
        socklen_t len = sizeof(sockaddr_in);
        char dns_name[NI_MAXHOST];
        sa_in.sin_family = AF_INET;
        memset(dns_name, 0, NI_MAXHOST);
        if (inet_aton(address.c_str(), &sa_in.sin_addr) == 1) {
            if (!getnameinfo(sa, len, dns_name, sizeof(dns_name), NULL, 0, NI_NAMEREQD)) {
                auto &it_ext_dns = ext.append("dns.1");
                it_ext_dns.value = dns_name;
                it_ext_dns.readOnly = false;
                logDebug("Retrieved DNS information: FQDN = '{}'", dns_name);
                char* p = strchr(dns_name, '.');
                if (p) {
                    *p = 0;
                }
                auto &it_ext_hostname = ext.append("hostname");
                it_ext_hostname.value = dns_name;
                it_ext_hostname.readOnly = false;
                logDebug("Hostname = '{}'", dns_name);
            }
            else {
                return fty::unexpected("No host information retrieved from DNS");
            }
        }
        else {
            return fty::unexpected("Error during read DNS");
        }
    }
    else {
        return fty::unexpected("Ip address empty");
    }
    return {};
}

void AutoDiscovery::scan(const commands::discoveryauto::In in)
{
    std::string discoveryConfigFile("/etc/fty-discovery/fty-discovery.cfg");  // TBD
    zconfig_t* config = zconfig_load(discoveryConfigFile.c_str());
    if (!config) {
        logError("Failed to load config file {}", discoveryConfigFile);
        config = zconfig_new("root", NULL);
    }

    // Init default links TBD: To put in a function ???
    fty::asset::create::PowerLinks defaultValuesLinks;
    const char *CFG_DISCOVERY_DEFAULT_VALUES_LINKS("/defaultValuesLinks");
    auto section = zconfig_locate(config, CFG_DISCOVERY_DEFAULT_VALUES_LINKS);
    if (section) {
        for (auto link = zconfig_child(section); link; link = zconfig_next(link)) {
            std::string iname(zconfig_get(link, "src", ""));
            fty::asset::create::PowerLink powerLink;
            // When link to no source, the file will have "0"
            powerLink.source = iname;
            powerLink.link_type = fty::convert<uint32_t>(std::stoi(zconfig_get(link, "type", "1")));
            if (!(powerLink.source.value() == "0")) {
                defaultValuesLinks.append(powerLink);
            }
        }
    }
    logDebug("defaultValuesLinks size={}", defaultValuesLinks.size());

    // Init default parent TBD: To put in a function ???
    bool deviceCentricView = false;
    std::string defaultParent("0");
    const char *CFG_DISCOVERY_DEFAULT_VALUES_AUX("/defaultValuesAux");
    section = zconfig_locate(config, CFG_DISCOVERY_DEFAULT_VALUES_AUX);
    if (section) {
        for (auto item = zconfig_child(section); item; item = zconfig_next(item)) {
            std::string configName(zconfig_name(item));
            // the config API call returns the parent internal name, while fty-proto-t needs to carry the database ID
            if (configName == "parent") {
                defaultParent = zconfig_value(item);
                if (defaultParent.empty()) {
                   defaultParent == "0";
                }
                if (defaultParent != "0") {
                    deviceCentricView = true;
                }
            }
        }
    }
    logDebug("defaultParent={}", defaultParent);

    // Get list of protocols
    commands::protocols::In in_prot;
    in_prot.address = in.address;
    Protocols protocols;
    auto list_protocols = protocols.getProtocols(in_prot);
    if (!list_protocols) {
        logError(list_protocols.error().c_str());
        return;
    }
    std::ostringstream list_str;
    std::for_each((*list_protocols).begin(), (*list_protocols).end(),
        [&list_str](const fty::job::Type& type) {
          list_str << Protocols::getProtocolStr(type) << " ";
        }
    );
    logInfo("Found protocols [ {}]", list_str.str());

    // For each protocols read, get assets list available
    // Note: stop when found first assets with the protocol in progress
    for (const auto& prot : *list_protocols) {
        logInfo("Try with protocol {}: ", Protocols::getProtocolStr(prot));

        commands::assets::In in_asset(in);
        in_asset.protocol = Protocols::getProtocolStr(prot);

        logInfo("in_asset address={}", in_asset.address);

        commands::assets::Out out_asset;
        Assets assets;
        if (auto getAssetsRes = assets.getAssets(in_asset, out_asset)) {
            logInfo("Found asset with protocol {}", Protocols::getProtocolStr(prot));

            auto getStatus = [deviceCentricView]() -> uint {
                return deviceCentricView ? static_cast<uint>(fty::AssetStatus::Active) : static_cast<uint>(fty::AssetStatus::Nonactive);
            };

            // Create asset list
            fty::disco::MessageBus bus;
            constexpr const char* agent = "fty-discovery-ng";
            if (auto init = bus.init(agent); !init) {
                logError(init.error());
                return;
            }

            std::string assetNameCreated;

            // For each asset to create
            for (auto& asset : out_asset) {
                fty::asset::create::Request req;
                req.type = asset.asset.type;
                req.sub_type = asset.asset.subtype;
                auto &ext = asset.asset.ext;
                req.status = getStatus();
                req.priority = 3;
                req.linked = defaultValuesLinks;
                req.parent = (defaultParent == "0") ? "" : defaultParent;
                // Initialise ext attributes
                if (auto res = updateExt(ext, req.ext); !res) {
                    logError("Could not update ext during creation of asset: {}", res.error());
                }
                // Update host name
                if (auto res = updateHostName(in.address, req.ext); !res) {
                    logError("Could not update host name during creation of asset: {}", res.error());
                }
                // Create asset
                if (auto res = fty::asset::create::run(bus, agent, req); !res) {
                    logError("Could not create asset: {}", res.error());
                    continue;
                }
                else {
                    assetNameCreated = res->name;
                    logInfo("Create asset: name={}", assetNameCreated);
                }

                // Now create sensors attached to the asset
                for (auto& sensor : asset.sensors) {
                    // TBD add a function ???
                    fty::asset::create::Request req_sensor;
                    req_sensor.type = sensor.type;
                    req_sensor.sub_type = sensor.subtype;
                    req_sensor.status = getStatus();
                    req_sensor.priority = 3;
                    req_sensor.linked = {}; //defaultValuesLinks; // TBD ???
                    req_sensor.parent = assetNameCreated;

                    // Add logical asset in ext
                    auto ext_logical_asset = sensor.ext.append();
                    ext_logical_asset.append("logical_asset", (defaultParent == "0") ? "" : defaultParent);
                    ext_logical_asset.append("read_only", "false");
                    // Add parent name in ext
                    auto ext_parent_name = sensor.ext.append();
                    ext_parent_name.append("parent_name.1", assetNameCreated);
                    ext_parent_name.append("read_only", "false");

                    // Initialise ext attributes
                    if (auto res = updateExt(sensor.ext, req_sensor.ext); !res) {
                        logError("Could not update ext during creation of sensor: {}", res.error());
                    }
                    if (auto res_sensor = fty::asset::create::run(bus, agent, req_sensor); !res_sensor) {
                        logError("Could not create sensor: {}", res_sensor.error());
                        continue;
                    }
                }
            }
            // Found assets with protocol, we can leave
            break;
        }
        else {
            logError(getAssetsRes.error().c_str());
        }
    }
}

void AutoDiscovery::run(const commands::discoveryauto::In& in, commands::discoveryauto::Out&)
{
    m_params = in;

    // Test input parameter
    // TBD
    bool res = true;
    if (!res) {
        throw Error("Bad input parameter");
    }

    // Execute discovery task
    m_pool.pushWorker(scan, in/*, out*/);

    // no output
}

} // namespace fty::job
