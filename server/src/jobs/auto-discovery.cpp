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

Expected<void> AutoDiscovery::updateExt(const commands::assets::Ext &extIn, fty::asset::create::Ext &extOut)
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
    for (const auto& it: extIn) {
        fty::asset::create::MapExtObj newMapExtObj;
        std::string keyVal;
        // for each attribute
        for (const auto& [key, value]: it) {
            if (key == "read_only") {
                newMapExtObj.readOnly = (value == "true") ? true : false;
            }
            else {
                // It is the name of the key and its associated value
                keyVal = key;
                newMapExtObj.value = value;
            }
        }
        if (!keyVal.empty()) {
            extOut.append(keyVal, newMapExtObj);
        }
    }
    return {};
}

Expected<void> AutoDiscovery::updateHostName(const std::string& address, fty::asset::create::Ext &ext)
{
    if (!address.empty()) {
        // Set host name
        struct sockaddr_in saIn;
        struct sockaddr* sa  = reinterpret_cast<sockaddr*>(&saIn);
        socklen_t len = sizeof(sockaddr_in);
        char dnsName[NI_MAXHOST];
        saIn.sin_family = AF_INET;
        memset(dnsName, 0, NI_MAXHOST);
        if (inet_aton(address.c_str(), &saIn.sin_addr) == 1) {
            if (!getnameinfo(sa, len, dnsName, sizeof(dnsName), NULL, 0, NI_NAMEREQD)) {
                auto &itExtDns = ext.append("dns.1");
                itExtDns.value = dnsName;
                itExtDns.readOnly = false;
                logDebug("Retrieved DNS information: FQDN = '{}'", dnsName);
                char* p = strchr(dnsName, '.');
                if (p) {
                    *p = 0;
                }
                auto &itExtHostname = ext.append("hostname");
                itExtHostname.value = dnsName;
                itExtHostname.readOnly = false;
                logDebug("Hostname = '{}'", dnsName);
            }
            else {
                return fty::unexpected("No host information retrieved from DNS for {}", address);
            }
        }
        else {
            return fty::unexpected("Error during read DNS for {}", address);
        }
    }
    else {
        return fty::unexpected("Ip address empty");
    }
    return {};
}

void AutoDiscovery::scan(const commands::discoveryauto::In in)
{
    /////////////////////////////////////////////////////////////////////
    // TBD: To be replaced
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
    /////////////////////////////////////////////////////////////////////

    // Get list of protocols
    commands::protocols::In inProt;
    inProt.address = in.address;
    Protocols protocols;
    auto listProtocols = protocols.getProtocols(inProt);
    if (!listProtocols) {
        logError(listProtocols.error());
        return;
    }
    std::ostringstream listStr;
    std::for_each((*listProtocols).begin(), (*listProtocols).end(),
        [&listStr](const fty::job::Type& type) {
          listStr << Protocols::getProtocolStr(type) << " ";
        }
    );
    logDebug("Found protocols [ {}]", listStr.str());

    // Init bus
    fty::disco::MessageBus bus;
    constexpr const char* agent = "fty-discovery-ng";
    if (auto init = bus.init(agent); !init) {
        logError(init.error());
        return;
    }

    // For each protocols read, get assets list available
    // Note: stop when found first assets with the protocol in progress
    for (const auto& prot : *listProtocols) {
        logInfo("Try with protocol {}: ", Protocols::getProtocolStr(prot));

        commands::assets::In inAsset(in);
        inAsset.protocol = Protocols::getProtocolStr(prot);

        logInfo("in_asset address={}", inAsset.address);

        commands::assets::Out outAsset;
        Assets assets;
        if (auto getAssetsRes = assets.getAssets(inAsset, outAsset)) {
            logInfo("Found asset with protocol {}", Protocols::getProtocolStr(prot));

            auto getStatus = [deviceCentricView]() -> uint {
                return deviceCentricView ? static_cast<uint>(fty::AssetStatus::Active) : static_cast<uint>(fty::AssetStatus::Nonactive);
            };

            // Create asset list
            std::string assetNameCreated;

            // For each asset to create
            for (auto& asset : outAsset) {
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
                    fty::asset::create::Request reqSensor;
                    reqSensor.type = sensor.type;
                    reqSensor.sub_type = sensor.subtype;
                    reqSensor.status = getStatus();
                    reqSensor.priority = 3;
                    reqSensor.linked = {}; //defaultValuesLinks; // TBD ???
                    reqSensor.parent = assetNameCreated;

                    // Add logical asset in ext
                    auto extLogicalAsset = sensor.ext.append();
                    extLogicalAsset.append("logical_asset", (defaultParent == "0") ? "" : defaultParent);
                    extLogicalAsset.append("read_only", "false");
                    // Add parent name in ext
                    auto extParentName = sensor.ext.append();
                    extParentName.append("parent_name.1", assetNameCreated);
                    extParentName.append("read_only", "false");

                    // Initialise ext attributes
                    if (auto res = updateExt(sensor.ext, reqSensor.ext); !res) {
                        logError("Could not update ext during creation of sensor: {}", res.error());
                    }
                    if (auto resSensor = fty::asset::create::run(bus, agent, reqSensor); !resSensor) {
                        logError("Could not create sensor: {}", resSensor.error());
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
    m_pool.pushWorker(scan, in);

    // no output
}

} // namespace fty::job
