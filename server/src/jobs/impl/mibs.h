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
#include <fty/expected.h>
#include <memory>
#include <set>
#include <string>

namespace fty::disco::impl {

// =====================================================================================================================

std::string mapMibToLegacy(const std::string& mib);

// =====================================================================================================================

namespace snmp {
    class Session;
    using SessionPtr = std::shared_ptr<Session>;
} // namespace snmp

/// Reads list of mibs from endpoint
class MibsReader
{
public:
    using MibList = std::set<std::string>;

    MibsReader(const std::string& address, uint16_t port);
    Expected<void> setCredentialId(const std::string& credentialId);
    Expected<void> setCommunity(const std::string& community);
    Expected<void> setTimeout(uint32_t miliseconds);

    Expected<MibList>     read() const;
    Expected<std::string> readName() const;

private:
    snmp::SessionPtr m_session;
    mutable bool     m_isOpen = false;
};

// =====================================================================================================================

} // namespace fty::disco::impl
