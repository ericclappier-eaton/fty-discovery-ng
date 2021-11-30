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

// =====================================================================================================================

namespace fty::job {

/// Automatic discover Assets from enpoint
/// Returns @ref commands::discoveryauto::Out (empty if no error in input parameter)
class AutoDiscovery : public Task<AutoDiscovery, disco::commands::scan::start::In, disco::commands::scan::start::Out>
{
public:
    using Task::Task;

    // Runs discover job.
    //void run(const commands::discoveryauto::In& in, commands::discoveryauto::Out& out);
    void run(const disco::commands::scan::start::In& in, disco::commands::scan::start::Out& out);
private:
    // Construct and update output ext attributes according input ext attributes
    static Expected<void> updateExt(const commands::assets::Ext& ext_in, fty::asset::create::Ext& ext_out);

    // Update host name
    static Expected<void> updateHostName(const std::string& address, fty::asset::create::Ext& ext);

    // Device centric view
    bool IsDeviceCentricView() const { return (m_params.parent == "0") ? false : true; };

    // Read configuration
    void readConfig(const disco::commands::scan::start::In& in, const disco::commands::scan::start::Out& out);

    // Scan node(s)
    static void scan(AutoDiscovery *autoDiscovery, const commands::discoveryauto::In& in);

    // Input parameters
    disco::commands::scan::start::In m_params;

    fty::asset::create::PowerLinks m_defaultValuesLinks;

    std::vector<std::string> m_listIpAddress;

    // Thread pool use for discovery scan
    fty::ThreadPool m_pool;
};

} // namespace fty::job

// =====================================================================================================================
