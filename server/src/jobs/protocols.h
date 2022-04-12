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

// =====================================================================================================================

namespace fty::disco::job {

struct config_protocol_t {
    ConfigDiscovery::Protocol::Type protocol;
    uint16_t                        defaultPort;
};

/// Discover supported protocols by endpoint
/// Returns @ref commands::protocols::Out (list of protocols)
class Protocols : public Task<Protocols, commands::protocols::In, commands::protocols::Out>
{
public:
    using Task::Task;

    // Get list protocols found
    Expected<commands::protocols::Out> getProtocols(const commands::protocols::In& in) const;

    /// Runs discover job.
    void run(const commands::protocols::In& in, commands::protocols::Out& out);

    // Test input option (protocol filter and port)
    static std::optional<const fty::disco::ConfigDiscovery::Protocol>
    findProtocol(const config_protocol_t config_protocol, const commands::protocols::In& in);

private:
    /// Try out if endpoint support xml pdc protocol
    Expected<void> tryXmlPdc(const std::string& address, uint16_t port) const;

    /// Try out if endpoint support xnmp protocol
    Expected<void> trySnmp(const std::string& address, uint16_t port) const;

    /// Try out if endpoint support genapi protocol
    Expected<void> tryPowercom(const std::string& address, uint16_t port) const;
};

} // namespace fty::disco::job

// =====================================================================================================================
