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

#pragma once
#include "discovery-task.h"
#include "impl/create-asset.h"
#include <fty/thread-pool.h>

// =====================================================================================================================

namespace fty::disco::job {

/// Automatic discover Assets from enpoint
class AutoDiscovery
{
public:
    static const uint32_t SCAN_CHECK_PERIOD_MS = 10000;

    // TBD: To centralise with rest status ???
    enum class State {
        Unknown,
        CancelledByUser,
        Terminated,
        InProgress
    };
    struct StatusDiscovery {
        State           state;
        uint32_t        progress;
        uint32_t        discovered;
        uint32_t        ups;
        uint32_t        epdu;
        uint32_t        sts;
        uint32_t        sensors;
    };

    AutoDiscovery();
    ~AutoDiscovery() = default;

    // Runs discover
    Expected<void> start(const disco::commands::scan::start::In& InStart);

    // Stop current discover
    Expected<void> stop();

    const StatusDiscovery& getStatus() { return m_statusDiscovery; };

private:
    // Construct and update output ext attributes according input ext attributes
    static Expected<void> updateExt(const commands::assets::Ext& ext_in, asset::create::Ext& ext_out);

    // Update host name
    static Expected<void> updateHostName(const std::string& address, asset::create::Ext& ext);

    // Split protocol and port number if present (optional)
    static const std::pair<std::string, std::string> splitPortFromProtocol(const std::string &protocol);

    // Device centric view
    bool IsDeviceCentricView() const
    {
        return (m_params.parent == "0") ? false : true;
    };

    // Read configuration
    void readConfig(const disco::commands::scan::start::In& in);

    // Status management
    void statusDiscoveryReset();
    void updateStatusDiscoveryCounters(std::string deviceSubType);
    void updateStatusDiscoveryProgress();

    // Scan node(s)
    static void scan(AutoDiscovery* autoDiscovery, const std::string& ipAddress);

    // Scan check
    static bool scanCheck(AutoDiscovery* autoDiscovery);
    static void startThreadScanCheck(AutoDiscovery* autoDiscovery, const unsigned int interval);

private:
    // Input parameters
    disco::commands::scan::start::In m_params;

    // Default power links
    fty::asset::create::PowerLinks   m_defaultValuesLinks;

    // Ip address list
    std::vector<std::string>         m_listIpAddress;

    // Ip address initial count
    uint64_t                         m_listIpAddressCount;

    // Automatic discovery status
    StatusDiscovery                  m_statusDiscovery;

    // Thread pool use for discovery scan
    fty::ThreadPool                  m_poolScan;

    // Mutex for secure auto discovery
    std::mutex                       m_mutex;
};

} // namespace fty::disco::job

// =====================================================================================================================
