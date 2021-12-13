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
#include "assets.h"
#include "protocols.h"
#include "src/config.h"
#include <asset/asset-manager.h>
#include <fty_asset_dto.h>
#include <fty_security_wallet.h>
#include <string>
#include <sys/types.h>

namespace fty::disco::job {

AutoDiscovery::AutoDiscovery() : m_poolScan(Config::instance().pollScanMax)
{
    statusDiscoveryReset();
}

Expected<void> AutoDiscovery::updateExt(const commands::assets::Ext& extIn, asset::create::Ext& extOut)
{
    // Construct and update output ext attributes according input ext attributes
    // [{ "key": "val", "read_only": "true" }, ...] -> {
    //                                                   "key:" {
    //                                                     "value": "val",
    //                                                     "readOnly: true,
    //                                                     "update": true
    //                                                   },
    //                                                   ...
    //                                                 }
    for (const auto& it : extIn) {
        asset::create::MapExtObj newMapExtObj;
        std::string              keyVal;
        // for each attribute
        for (const auto& [key, value] : it) {
            if (key == "read_only") {
                newMapExtObj.readOnly = (value == "true") ? true : false;
            } else {
                // It is the name of the key and its associated value
                keyVal             = key;
                newMapExtObj.value = value;
            }
        }
        if (!keyVal.empty()) {
            extOut.append(keyVal, newMapExtObj);
        }
    }
    return {};
}

Expected<void> AutoDiscovery::updateHostName(const std::string& address, asset::create::Ext& ext)
{
    if (!address.empty()) {
        // Set host name
        struct sockaddr_in saIn;
        struct sockaddr*   sa  = reinterpret_cast<sockaddr*>(&saIn);
        socklen_t          len = sizeof(sockaddr_in);
        char               dnsName[NI_MAXHOST];
        saIn.sin_family = AF_INET;
        memset(dnsName, 0, NI_MAXHOST);
        if (inet_aton(address.c_str(), &saIn.sin_addr) == 1) {
            if (!getnameinfo(sa, len, dnsName, sizeof(dnsName), NULL, 0, NI_NAMEREQD)) {
                auto& itExtDns    = ext.append("dns.1");
                itExtDns.value    = dnsName;
                itExtDns.readOnly = false;
                logDebug("Retrieved DNS information: FQDN = '{}'", dnsName);
                char* p = strchr(dnsName, '.');
                if (p) {
                    *p = 0;
                }
                auto& itExtHostname    = ext.append("hostname");
                itExtHostname.value    = dnsName;
                itExtHostname.readOnly = false;
                logDebug("Hostname = '{}'", dnsName);
            } else {
                return fty::unexpected("No host information retrieved from DNS for {}", address);
            }
        } else {
            return fty::unexpected("Error during read DNS for {}", address);
        }
    } else {
        return fty::unexpected("Ip address empty");
    }
    return {};
}

const std::pair<std::string, std::string> AutoDiscovery::splitPortFromProtocol(const std::string &str) {
    // split protocol and port with format "<protocol>:<port>"
    std::string protocol, port;
    auto pos = str.find(":");
    // If port found,
    if (pos != std::string::npos) {
        // get first protocol
        protocol = std::move(str.substr(0, pos -1));
        // and get second port number
        port = std::move(str.substr(pos + 1));
    }
    else {
        // No port found, return only protocol
        protocol = str;
    }
    return make_pair(protocol, port);
}

void AutoDiscovery::readConfig(const disco::commands::scan::start::In& in/*, const disco::commands::scan::start::Out& out*/)
{
    // Init default links
    asset::create::PowerLink powerLink;
    // For each link
    for (const auto& link : in.linkSrc) {
        // When link to no source, the file will have "0"
        if (!(link == "0")) {
            powerLink.source    = link;
            powerLink.link_type = 1; // TBD
            m_defaultValuesLinks.append(powerLink);
            logTrace("defaultValuesLinks add={}", link);
        }
    }
    logTrace("defaultValuesLinks size={}", m_defaultValuesLinks.size());

    // Init default parent
    std::string defaultParent = in.parent;
    if (defaultParent.empty()) {
        in.parent == "0";
    }
    logTrace("defaultParent={}", defaultParent);

    // TBD To be finished
    // For each ip on ips
    for (const auto& ip: in.ips) {
        m_listIpAddress.push_back(ip);
    }
}

void AutoDiscovery::statusDiscoveryReset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_statusDiscovery.ups = 0;
    m_statusDiscovery.epdu = 0;
    m_statusDiscovery.sts = 0;
    m_statusDiscovery.sensors = 0;
    m_statusDiscovery.discovered = 0;
    m_statusDiscovery.progress = 0;
}

void AutoDiscovery::updateStatusDiscoveryCounters(std::string deviceSubType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (deviceSubType == "ups") {
        m_statusDiscovery.ups++;
    }
    else if (deviceSubType == "epdu") {
        m_statusDiscovery.epdu++;
    }
    else if (deviceSubType == "sts") {
        m_statusDiscovery.sts++;
    }
    else if (deviceSubType == "sensor") {
        m_statusDiscovery.sensors++;
    }
    else {
        logError("Bad sub type discovered: {}", deviceSubType);
        return;
    }
    m_statusDiscovery.discovered++;
}

void AutoDiscovery::updateStatusDiscoveryProgress() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_listIpAddressCount != 0) {
        m_statusDiscovery.progress = static_cast<uint32_t>(std::round(100 *
            (m_listIpAddressCount - m_listIpAddress.size()) / m_listIpAddressCount));
    }
    else {
        m_statusDiscovery.progress = 100;
    }
}

void AutoDiscovery::scan(AutoDiscovery* autoDiscovery, const std::string& ipAddress)
{
    if (!autoDiscovery) return;

    // Get list of protocols
    commands::protocols::In inProt;
    inProt.address = ipAddress;
    Protocols protocols;
    auto      listProtocols = protocols.getProtocols(inProt);
    if (!listProtocols) {
        logError(listProtocols.error());
        // Update progress
        autoDiscovery->updateStatusDiscoveryProgress();
        return;
    }
    std::ostringstream listStr;
    std::for_each((*listProtocols).begin(), (*listProtocols).end(), [&listStr](const job::Type& type) {
        listStr << Protocols::getProtocolStr(type) << " ";
    });
    logDebug("Found protocols [ {}] for {}", listStr.str(), ipAddress);

    // Init bus
    std::string agent = "fty-discovery-ng" + std::to_string(gettid());
    fty::disco::MessageBus bus;
    if (auto init = bus.init(agent.c_str()); !init) {
        logError(init.error());
        return;
    }

    Assets assets;

    bool found = false;
    // For each protocols read, get assets list available
    // Note: stop when found first assets with the protocol in progress
    for (const auto& protocol : *listProtocols) {
        std::string strPort;
        auto strProtocol = Protocols::getProtocolStr(protocol);

        // Test if protocol tested is requested
        auto& listProtocolRequested = autoDiscovery->m_params.protocols;
        if (!listProtocolRequested.empty()) {
            if (auto it = std::find_if(listProtocolRequested.begin(), listProtocolRequested.end(),
                    [&strProtocol, &strPort](std::string protocolRequested) {
                        auto split = AutoDiscovery::splitPortFromProtocol(protocolRequested);
                        if (split.first == strProtocol) {
                            if (!split.second.empty()) {
                                strPort = split.second;
                            }
                            return true;
                        }
                        return false;
                    }); it == listProtocolRequested.end()) {
                logInfo("Skip protocol {} for {}", strProtocol, ipAddress);
                continue;
            }
        }

        // Get list of credentials
        auto& documents = autoDiscovery->m_params.documents;

        // For each credential, try to discover assets according the protocol in progress
        // Note: stop when found first assets with the credential in progress
        for (const auto doc : documents) {
            logInfo("Try with protocol/credential ({}/{}) for {}", strProtocol, doc, ipAddress);
            commands::assets::In inAsset;
            inAsset.address = ipAddress;
            inAsset.protocol = strProtocol;
            if (!strPort.empty()) {
                inAsset.port = static_cast<uint32_t>(std::stoul(strPort.c_str()));
            }
            inAsset.settings.credentialId = doc;

            commands::assets::Out outAsset;
            if (auto getAssetsRes = assets.getAssets(inAsset, outAsset)) {
                logInfo("Found asset with protocol/credential ({}/{}) for {}", strProtocol, doc, ipAddress);

                auto getStatus = [autoDiscovery]() -> uint {
                    return autoDiscovery->IsDeviceCentricView() ? static_cast<uint>(fty::AssetStatus::Active)
                                                                : static_cast<uint>(fty::AssetStatus::Nonactive);
                };

                // Create asset list
                std::string assetNameCreated;

                // For each asset to create
                for (auto& asset : outAsset) {
                    asset::create::Request req;
                    req.type     = asset.asset.type;
                    req.sub_type = asset.asset.subtype;
                    auto& ext    = asset.asset.ext;
                    req.status   = getStatus();
                    req.priority = 3;
                    req.linked   = autoDiscovery->m_defaultValuesLinks;
                    req.parent   = (autoDiscovery->m_params.parent == "0") ? pack::String(std::string(""))
                                                                           : autoDiscovery->m_params.parent;
                    // Initialise ext attributes
                    if (auto res = updateExt(ext, req.ext); !res) {
                        logError("Could not update ext during creation of asset ({}): {}", ipAddress, res.error());
                    }
                    // Update host name
                    if (auto res = updateHostName(ipAddress, req.ext); !res) {
                        logError("Could not update host name during creation of asset ({}): {}", ipAddress, res.error());
                    }
                    // Create asset
                    if (auto res = asset::create::run(bus, Config::instance().actorName, req); !res) {
                        logError("Could not create asset ({}): {}", ipAddress, res.error());
                        continue;
                    } else {
                        assetNameCreated = res->name;
                        logInfo("Create asset for {}: name={}", ipAddress, assetNameCreated);
                    }
                    autoDiscovery->updateStatusDiscoveryCounters(asset.asset.subtype);

                    // Now create sensors attached to the asset
                    for (auto& sensor : asset.sensors) {
                        // TBD add a function ???
                        asset::create::Request reqSensor;
                        reqSensor.type     = sensor.type;
                        reqSensor.sub_type = sensor.subtype;
                        reqSensor.status   = getStatus();
                        reqSensor.priority = 3;
                        reqSensor.linked   = {}; // defaultValuesLinks; // TBD ???
                        reqSensor.parent   = assetNameCreated;

                        // Add logical asset in ext
                        auto& extLogicalAsset = sensor.ext.append();
                        extLogicalAsset.append("logical_asset", (autoDiscovery->m_params.parent == "0")
                                                                ? pack::String(std::string(""))
                                                                : autoDiscovery->m_params.parent);
                        extLogicalAsset.append("read_only", "false");
                        // Add parent name in ext
                        auto& extParentName = sensor.ext.append();
                        extParentName.append("parent_name.1", assetNameCreated);
                        extParentName.append("read_only", "false");

                        // Initialise ext attributes
                        if (auto res = updateExt(sensor.ext, reqSensor.ext); !res) {
                            logError("Could not update ext during creation of sensor ({}): {}", ipAddress, res.error());
                        }
                        if (auto resSensor = asset::create::run(bus, Config::instance().actorName, reqSensor); !resSensor) {
                            logError("Could not create sensor ({}): {}", ipAddress, resSensor.error());
                            continue;
                        }
                        autoDiscovery->updateStatusDiscoveryCounters(sensor.subtype);
                    }
                }
                // Found assets with protocol and credential, we can leave
                found = true;
                break;
            } else {
                logError(getAssetsRes.error().c_str());
            }
        }
        // If found, leave protocol boucle
        if (found) {
            break;
        }
    }
    // Update progress
    autoDiscovery->updateStatusDiscoveryProgress();
}

bool AutoDiscovery::scanCheck(AutoDiscovery* autoDiscovery) {
    if (!autoDiscovery) return true;

    logTrace("getCountActiveTasks={}", autoDiscovery->m_poolScan.getCountActiveTasks());
    if (autoDiscovery->m_poolScan.getCountActiveTasks() == 0) {

        std::lock_guard<std::mutex> lock(autoDiscovery->m_mutex);
        autoDiscovery->m_statusDiscovery.state = State::Terminated;
        return true;
    }
    return false;
}

void AutoDiscovery::startThreadScanCheck(AutoDiscovery* autoDiscovery, const unsigned int interval) {
    if (!autoDiscovery) return;

    logTrace("Start of scan thread");
    std::thread([autoDiscovery, interval]()
    {
        bool isEnd = false;
        while (!isEnd)
        {
            auto time = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);
            isEnd = AutoDiscovery::scanCheck(autoDiscovery);
            std::this_thread::sleep_until(time);
        }
        logTrace("End of scan thread");
    }).detach();
}

Expected<void> AutoDiscovery::start(const disco::commands::scan::start::In& in)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_statusDiscovery.state != State::InProgress) {
        m_statusDiscovery.state = State::InProgress;
        lock.unlock();
        m_params = in;

        // Test input parameter
        // TBD
        bool res = true;
        if (!res) {
            return fty::unexpected("Bad input parameter");
        }

        // Read input parameters
        readConfig(in);

        // Init scan counters
        m_listIpAddressCount = m_listIpAddress.size();
        statusDiscoveryReset();

        // For each ip, execute scan discovery
        for (auto it = m_listIpAddress.begin(); it != m_listIpAddress.end(); it++) {
            // Execute discovery task
            m_poolScan.pushWorker(scan, this, *it);
            logTrace("Add scan with ip {}", *it);
            m_listIpAddress.erase(it--);
        }
        // Execute task in charge of test end of discovery
        AutoDiscovery::startThreadScanCheck(this, SCAN_CHECK_PERIOD_MS);
    }
    else {
        return fty::unexpected("Scan in progress");
    }
    return {};
}

Expected<void> AutoDiscovery::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_statusDiscovery.state == State::InProgress) {
        m_statusDiscovery.state = State::CancelledByUser;
        m_poolScan.stop(fty::ThreadPool::Stop::Immedialy);
    }
    else {
        return fty::unexpected("No scan in progress");
    }
    return {};
}

} // namespace fty::disco::job
