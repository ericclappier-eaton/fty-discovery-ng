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
#include "create-asset.h"
#include <fty/thread-pool.h>

// =====================================================================================================================

namespace fty::disco::job {

using StatusDiscovery = commands::scan::status::Out;

/// Automatic discover Assets from enpoint
class AutoDiscovery
{
public:
    static const uint32_t SCAN_CHECK_PERIOD_MS = 1000;
    static const uint32_t SCAN_MIN_NUM_THREAD  = 5;

    enum class AssetStatus
    {
        Unknown = 0,
        Active,
        Nonactive
    };

    AutoDiscovery();
    ~AutoDiscovery();
    void shutdown();

    // Init discover
    Expected<void> init();

    // Runs discover
    Expected<void> start(const disco::commands::scan::start::In& in);

    // Stop current discover
    Expected<void> stop();

//private:
    // Device centric view
    bool isDeviceCentricView() const
    {
        return (m_params.aux.parent == "0") ? true : false;
    };

    // Read configuration
    Expected<void> readConfig();

    // Status management
    const StatusDiscovery& getStatus() { return m_statusDiscovery; };
    void statusDiscoveryInit();
    void statusDiscoveryReset(uint32_t numOfAddress);
    void updateStatusDiscoveryCounters(const std::string& deviceSubType);
    void updateStatusDiscoveryProgress();

    // Scan node(s)
    static void scan(AutoDiscovery* autoDiscovery, const std::string& ipAddress);

    // Scan check
    static bool scanCheck(AutoDiscovery* autoDiscovery);
    static void startThreadScanCheck(AutoDiscovery* autoDiscovery, const unsigned int interval);

    // Manage pool scan
    void resetPoolScan();
    void stopPoolScan();

    // Construct and update output ext attributes according input ext attributes
    static Expected<void> updateExt(const commands::assets::Ext& ext_in, asset::create::Ext& ext_out);

    // Update host name
    static Expected<void> updateHostName(const std::string& address, asset::create::Ext& ext);

private:
    // Input parameters
    ConfigDiscovery                  m_params;

    // Default power links
    asset::create::PowerLinks        m_defaultValuesLinks;

    // Ip address list
    std::vector<std::string>         m_listIpAddress;

    // Automatic discovery status
    StatusDiscovery                  m_statusDiscovery;

    // Thread pool use for discovery scan
    std::unique_ptr<fty::ThreadPool> m_poolScan;

    // Mutex for secure auto discovery
    std::mutex                       m_mutex;

    // Bus for creation asset
    disco::MessageBus                m_bus;
};

} // namespace fty::disco::job

// =====================================================================================================================
