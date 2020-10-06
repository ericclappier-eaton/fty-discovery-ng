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
#include <functional>
#include <memory>

namespace fty::protocol {

namespace snmp {
    class Session;
    using SessionPtr = std::shared_ptr<Session>;
}

// =====================================================================================================================

class Snmp
{
public:
public:
    ~Snmp();
    static Snmp&     instance();
    snmp::SessionPtr session(const std::string& address, uint16_t port);
    void             init(const std::string& mibsPath);

private:
    Snmp();
};

// =====================================================================================================================

namespace snmp {
    class Session
    {
    public:
        void setCommunity(const std::string& community);
        void setCredentialId(const std::string& credId);
        void setTimeout(uint32_t milliseconds);

        Expected<void>        open();
        Expected<std::string> read(const std::string& oid) const;
        Expected<void>        walk(std::function<void(const std::string&)>&& func) const;

    protected:
        Session(const std::string& address, uint16_t port);

    private:
        friend class protocol::Snmp;
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace snmp


} // namespace fty::protocol
