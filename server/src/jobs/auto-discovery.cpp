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
#include "discovery-config-manager.h"
#include "assets.h"
#include "protocols.h"
#include "src/config.h"
#include "impl/address.h"
#include <asset/asset-manager.h>
#include <string>
#include <sys/types.h>
#include <fty_log.h>

namespace fty::disco::job {

AutoDiscovery::AutoDiscovery()
{
}

AutoDiscovery::~AutoDiscovery()
{
    shutdown();
}

void AutoDiscovery::shutdown()
{
    stop();
    m_poolScan->stop();
    m_bus.shutdown();
}

Expected<void> AutoDiscovery::init()
{
    // Init thread number
    size_t minNumThreads = SCAN_MIN_NUM_THREAD;
    size_t maxNumThreads = Config::instance().pollScanMax.value();
    if (maxNumThreads < minNumThreads) {
        maxNumThreads = minNumThreads;
        logInfo("AutoDiscovery: change scan max thread to {}", maxNumThreads);
    }
    logDebug("AutoDiscovery: create pool scan thread with min={}, max={}", minNumThreads, maxNumThreads);
    m_poolScan = std::unique_ptr<fty::ThreadPool>(new fty::ThreadPool(minNumThreads, maxNumThreads));

    // Init bus for asset creation
    std::string agent = "fty-discovery-ng-asset-creation";
    logTrace("Create agent {} with {} endpoint", agent, Config::instance().endpoint.value());
    if (auto init = m_bus.init(agent.c_str(), Config::instance().endpoint.value()); !init) {
        return fty::unexpected("Init bus error: {}", init.error());
    }
    // Init status
    statusDiscoveryInit();
    return {};
}

Expected<void> AutoDiscovery::start(const disco::commands::scan::start::In& in)
{
    //critical section to check the status
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_statusDiscovery.status == StatusDiscovery::Status::InProgess) {
            return fty::unexpected("Scan in progress");
        }
    }

    m_params = in;
    logTrace("Set scan in progress");

    // Read and test input parameters
    if (auto res = readConfig(); !res) {
        return fty::unexpected("Bad input parameter: {}", res.error());
    }

    // Init discovery
    statusDiscoveryReset(static_cast<uint32_t>(m_listIpAddress.size()));
    resetPoolScan();

    // For each ip, execute scan discovery
    for (auto it = m_listIpAddress.begin(); it != m_listIpAddress.end(); it++) {
        // Execute discovery task
        m_poolScan->pushWorker(scan, this, *it);
        logTrace("Add scan with ip {}", *it);
    }
    m_listIpAddress.clear();

    // Execute task in charge of test end of discovery
    AutoDiscovery::startThreadScanCheck(this, SCAN_CHECK_PERIOD_MS);

    logTrace("End start scan");
    return {};
}

Expected<void> AutoDiscovery::stop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_statusDiscovery.status == StatusDiscovery::Status::InProgess) {
        m_statusDiscovery.status = StatusDiscovery::Status::CancelledByUser;
        lock.unlock();
        stopPoolScan();
    }
    else {
        lock.unlock();
        return fty::unexpected("No scan in progress");
    }
    return {};
}

Expected<void> AutoDiscovery::readConfig()
{
    // Init default links
    m_defaultValuesLinks.clear();
    asset::create::PowerLink powerLink;
    // For each link
    for (const auto& link : m_params.links) {
        // When link to no source, the file will have "0"
        if (!(link.src == "0")) {
            powerLink.source    = link.src;
            powerLink.link_type = link.type; // TBD
            m_defaultValuesLinks.append(powerLink);
            logTrace("defaultValuesLinks add={}", link);
        }
    }
    logTrace("defaultValuesLinks size={}", m_defaultValuesLinks.size());

    m_listIpAddress.clear();
    // Ip list scan
    if (m_params.discovery.type == ConfigDiscovery::Discovery::Type::Ip) {
        if (m_params.discovery.ips.size() == 0) {
            fty::unexpected("Ips list empty");
        }
        for (const auto& ip : m_params.discovery.ips) {
            m_listIpAddress.push_back(ip);
        }
    }
    // Multi scan
    else if (m_params.discovery.type == ConfigDiscovery::Discovery::Type::Multi) {
        if (m_params.discovery.scans.size() == 0) {
            fty::unexpected("Scans list empty");
        }
        for (const auto& range : m_params.discovery.scans) {
            if (auto listIp = address::AddressParser::getRangeIp(range); listIp) {
                m_listIpAddress.insert(m_listIpAddress.end(), listIp->begin(), listIp->end());
            }
            else {
                logError("getRangeIp: {}", listIp.error());
            }
        }
    }
    // Full scan
    else if (m_params.discovery.type == ConfigDiscovery::Discovery::Type::Full) {

        if (m_params.discovery.ips.size() == 0 && m_params.discovery.scans.size() == 0) {
            fty::unexpected("Ips and scans list empty");
        }
        if (m_params.discovery.ips.size() > 0) {
            for (const auto& ip : m_params.discovery.ips) {
                m_listIpAddress.push_back(ip);
            }
        }
        if (m_params.discovery.scans.size() > 0) {
            for (const auto& range: m_params.discovery.scans) {
                if (auto listIp = address::AddressParser::getRangeIp(range); listIp) {
                    m_listIpAddress.insert(m_listIpAddress.end(), listIp->begin(), listIp->end());
                }
                else {
                    logError("getRangeIp: {}", listIp.error());
                }
            }
        }
    }
    // Local scan
    else if (m_params.discovery.type == ConfigDiscovery::Discovery::Type::Local) {
        if (auto listIp = address::AddressParser::getLocalRangeIp(); listIp) {
            m_listIpAddress.insert(m_listIpAddress.end(), listIp->begin(), listIp->end());
        }
        else {
            logError("getLocalRangeIp: {}", listIp.error());
        }
    }
    else {
        fty::unexpected("Bad scan type {}", m_params.discovery.type);
    }
    return {};
}

void AutoDiscovery::statusDiscoveryInit()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_statusDiscovery.status         = StatusDiscovery::Status::Unknown;
    m_statusDiscovery.numOfAddress   = 0;
    m_statusDiscovery.addressScanned = 0;
    m_statusDiscovery.discovered     = 0;
    m_statusDiscovery.ups            = 0;
    m_statusDiscovery.epdu           = 0;
    m_statusDiscovery.sts            = 0;
    m_statusDiscovery.sensors        = 0;
}

void AutoDiscovery::statusDiscoveryReset(uint32_t numOfAddress)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_statusDiscovery.status         = StatusDiscovery::Status::InProgess;
    m_statusDiscovery.numOfAddress   = numOfAddress;
    m_statusDiscovery.addressScanned = 0;
    m_statusDiscovery.discovered     = 0;
    m_statusDiscovery.ups            = 0;
    m_statusDiscovery.epdu           = 0;
    m_statusDiscovery.sts            = 0;
    m_statusDiscovery.sensors        = 0;
}

void AutoDiscovery::updateStatusDiscoveryCounters(const std::string& deviceSubType)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (deviceSubType == "ups") {
        m_statusDiscovery.ups = m_statusDiscovery.ups + 1;
    }
    else if (deviceSubType == "epdu") {
        m_statusDiscovery.epdu = m_statusDiscovery.epdu + 1;
    }
    else if (deviceSubType == "sts") {
        m_statusDiscovery.sts = m_statusDiscovery.sts + 1;
    }
    else if (deviceSubType == "sensor") {
        m_statusDiscovery.sensors = m_statusDiscovery.sensors + 1;
    }
    else {
        logError("Bad sub type discovered: {}", deviceSubType);
        return;
    }
    m_statusDiscovery.discovered = m_statusDiscovery.discovered + 1;
}

void AutoDiscovery::updateStatusDiscoveryProgress()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_statusDiscovery.addressScanned < m_statusDiscovery.numOfAddress) {
        m_statusDiscovery.addressScanned = m_statusDiscovery.addressScanned + 1;
    }
}

void AutoDiscovery::scan(AutoDiscovery* autoDiscovery, const std::string& ipAddress)
{
    try {
        logTrace("Start of scan thread");
        if (!autoDiscovery) {
            // Update progress
            logTrace("exit #0");
            autoDiscovery->updateStatusDiscoveryProgress();
            return;
        }

        // Get list of protocols
        commands::protocols::In inProt;
        inProt.address = ipAddress;
        // Optional parameter
        inProt.protocols = autoDiscovery->m_params.discovery.protocols;

        Protocols protocols;
        auto listProtocols = protocols.getProtocols(inProt);
        if (!listProtocols) {
            logError(listProtocols.error());
            // Update progress
            autoDiscovery->updateStatusDiscoveryProgress();
            return;
        }
        if (auto listStr = pack::json::serialize(*listProtocols, pack::Option::WithDefaults)) {
            logDebug("Found protocols for {}:\n{}", ipAddress, *listStr);
        }

        bool found = false;
        Assets assets;

        // For each protocols read, get assets list available
        // Note: stop when found first assets with the protocol in progress
        for (const auto& elt : *listProtocols) {
            // if protocol reachable
            if (elt.reachable) {

                // Get list of credentials
                auto& documents = autoDiscovery->m_params.discovery.documents;

                // For each credential, try to discover assets according the protocol in progress
                // Note: stop when found first assets with the credential in progress
                for (const auto doc : documents) {
                    logInfo("Try with protocol/port/credential ({}/{}/{}) for {}", elt.protocol, elt.port, doc, ipAddress);
                    commands::assets::In inAsset;
                    inAsset.address = ipAddress;
                    inAsset.protocol = elt.protocol;
                    inAsset.port = elt.port;
                    inAsset.settings.credentialId = doc;
                    // Optional parameter (for test) TBD
                    /*if (autoDiscovery->m_params.mib.hasValue()) {
                        logDebug("Set mib {}", autoDiscovery->m_params.mib);
                        inAsset.settings.mib = autoDiscovery->m_params.mib;
                    }*/
                    commands::assets::Out outAsset;
                    if (auto getAssetsRes = assets.getAssets(inAsset, outAsset)) {
                        logInfo("Found asset with protocol/port/credential ({}/{}/{}) for {}", elt.protocol, elt.port, doc, ipAddress);

                        auto getAssetStatus = [autoDiscovery]() -> uint {
                            return autoDiscovery->m_params.aux.status == "active" ?
                                static_cast<uint>(AssetStatus::Active) : static_cast<uint>(AssetStatus::Nonactive);
                        };

                        // Create asset list
                        std::string assetNameCreated;

                        // For each asset to create
                        for (auto& asset : outAsset) {
                            asset::create::Request req;
                            req.type     = asset.asset.type;
                            req.sub_type = asset.asset.subtype;
                            auto& ext    = asset.asset.ext;
                            req.status   = getAssetStatus();
                            req.priority = autoDiscovery->m_params.aux.priority;
                            req.linked   = autoDiscovery->m_defaultValuesLinks;
                            req.parent   = std::string(autoDiscovery->m_params.aux.parent);
                            // Initialise ext attributes
                            if (auto res = updateExt(ext, req.ext); !res) {
                                logError("Could not update ext during creation of asset ({}): {}", ipAddress, res.error());
                            }
                            // Update host name
                            if (auto res = updateHostName(ipAddress, req.ext); !res) {
                                logError("Could not update host name during creation of asset ({}): {}", ipAddress, res.error());
                            }
                            // Create asset
                            if (auto res = asset::create::run(autoDiscovery->m_bus, Config::instance().actorName.value(), req); !res) {
                                logError("Could not create asset ({}): {}", ipAddress, res.error());
                                continue;
                            } else {
                                assetNameCreated = res->name;
                                logInfo("Create asset for {}: name={}", ipAddress, assetNameCreated);
                            }
                            autoDiscovery->updateStatusDiscoveryCounters(asset.asset.subtype);

                            // Now create sensors attached to the asset
                            for (auto& sensor : asset.sensors) {
                                asset::create::Request reqSensor;
                                reqSensor.type     = sensor.type;
                                reqSensor.sub_type = sensor.subtype;
                                reqSensor.status   = getAssetStatus();
                                reqSensor.priority = autoDiscovery->m_params.aux.priority;
                                reqSensor.linked   = {}; // defaultValuesLinks; // TBD ???
                                reqSensor.parent   = assetNameCreated;

                                // Add logical asset in ext
                                auto& extLogicalAsset = sensor.ext.append();
                                extLogicalAsset.append("logical_asset", (autoDiscovery->m_params.aux.parent == "0") ?
                                    pack::String(std::string("")) : autoDiscovery->m_params.aux.parent);
                                extLogicalAsset.append("read_only", "false");
                                // Add parent name in ext
                                auto& extParentName = sensor.ext.append();
                                extParentName.append("parent_name.1", assetNameCreated);
                                extParentName.append("read_only", "false");

                                // Initialise ext attributes
                                if (auto res = updateExt(sensor.ext, reqSensor.ext); !res) {
                                    logError("Could not update ext during creation of sensor ({}): {}", ipAddress, res.error());
                                }
                                if (auto resSensor = asset::create::run(autoDiscovery->m_bus, Config::instance().actorName.value(), reqSensor); !resSensor) {
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
                // If found, leave main protocol loop
                if (found) {
                    break;
                }
            }
        }
    }
    catch (std::exception& ex) {
        logError(ex.what());
    }
    // Update progress
    autoDiscovery->updateStatusDiscoveryProgress();

}

bool AutoDiscovery::scanCheck(AutoDiscovery* autoDiscovery)
{
    if (!autoDiscovery) return true;

#undef WORK_AROUND_SCAN_BLOCKING

#if WORK_AROUND_SCAN_BLOCKING
    static size_t previousCounter = 0;
    static std::chrono::steady_clock::time_point start;
    static bool isBlockingDetected = false;
#endif

    auto countPendingTasks = autoDiscovery->m_poolScan->getCountPendingTasks();
    auto countActiveTasks = autoDiscovery->m_poolScan->getCountActiveTasks();

    logTrace("AutoDiscovery scanCheck: status ={}, pending tasks={}, active tasks={})",
        autoDiscovery->m_statusDiscovery.status, countPendingTasks, countActiveTasks);
    if (countPendingTasks == 0 && countActiveTasks == 0) {
        std::lock_guard<std::mutex> lock(autoDiscovery->m_mutex);
        autoDiscovery->m_statusDiscovery.status = StatusDiscovery::Status::Terminated;
        logTrace("End of discovery detected");
#if WORK_AROUND_SCAN_BLOCKING
        if (isBlockingDetected) {
            isBlockingDetected = false;
        }
#endif
        return true;
    }
    // TBD: Workaround for scan blocking: if counter don't decrease during 60 sec, stop scan in progress
#if WORK_AROUND_SCAN_BLOCKING
    const uint32_t TIMEOUT_BLOCKING_SCAN_SEC = 60;
    size_t counter = countPendingTasks + countActiveTasks;
    if (previousCounter == counter) {
        if (!isBlockingDetected) {
            isBlockingDetected = true;
            start = std::chrono::steady_clock::now();
        }
        else {
            auto end = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > TIMEOUT_BLOCKING_SCAN_SEC) {
                autoDiscovery->stopPoolScan();
                std::lock_guard<std::mutex> lock(autoDiscovery->m_mutex);
                autoDiscovery->m_statusDiscovery.state = Status::Terminated;
                logWarn("Blocking scan detected (Timeout of {} sec). Stop scan with {} remaining tasks",
                    TIMEOUT_BLOCKING_SCAN_SEC, countActiveTasks);
                return true;
            }
        }
    }
    else {
        if (isBlockingDetected) {
            isBlockingDetected = false;
        }
    }
#endif
    return false;
}

void AutoDiscovery::startThreadScanCheck(AutoDiscovery* autoDiscovery, const unsigned int interval)
{
    if (!autoDiscovery) return;

    logTrace("Start of scan check thread");
    std::thread([autoDiscovery, interval]()
    {
        bool isEnd = false;
        while (!isEnd)
        {
            isEnd = AutoDiscovery::scanCheck(autoDiscovery);
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
        logTrace("End of scan thread");
    }).detach();
}

void AutoDiscovery::resetPoolScan()
{
    m_poolScan.reset(new fty::ThreadPool(Config::instance().pollScanMax.value()));
}

void AutoDiscovery::stopPoolScan()
{
    m_poolScan->stop(fty::ThreadPool::Stop::Immedialy);
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
                newMapExtObj.readOnly = (value == "true");
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
        memset(dnsName, 0, sizeof(dnsName));
        if (inet_aton(address.c_str(), &saIn.sin_addr) == 1) {
            if (!getnameinfo(sa, len, dnsName, sizeof(dnsName), NULL, 0, NI_NAMEREQD)) {
                auto& itExtDns    = ext.append("dns.1");
                itExtDns.value    = std::string(dnsName);
                itExtDns.readOnly = false;
                logDebug("Retrieved DNS information: FQDN = '{}'", dnsName);
                char* p = strchr(dnsName, '.');
                if (p) {
                    *p = 0;
                }
                auto& itExtHostname    = ext.append("hostname");
                itExtHostname.value    = std::string(dnsName);
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

} // namespace fty::disco::job
