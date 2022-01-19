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

namespace fty::job {

/// Forward declaration
enum class Type;

/// Discover supported protocols by endpoint
/// Returns @ref commands::protocols::Out (list of protocols)
class Protocols : public Task<Protocols, commands::protocols::In, commands::protocols::Out>
{
public:
    using Task::Task;

    /// Runs discover job.
    void run(const commands::protocols::In& in, commands::protocols::Out& out);

private:
    /// Try out if endpoint support xml pdc protocol
    Expected<void> tryXmlPdc(const commands::protocols::In& in, uint16_t port) const;

    /// Try out if endpoint support xnmp protocol
    Expected<void> trySnmp(const commands::protocols::In& in, uint16_t port) const;

    /// Try out if endpoint support genapi protocol
    Expected<void> tryPowercom(const commands::protocols::In& in, uint16_t port) const;
};

} // namespace fty::job

// =====================================================================================================================
