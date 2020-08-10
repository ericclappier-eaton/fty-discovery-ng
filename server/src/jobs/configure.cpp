/*  =========================================================================
    configure.cpp - Mibs discovery job

    Copyright (C) 2014 - 2020 Eaton

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
    =========================================================================
 */

#include "configure.h"
#include "common.h"
#include "commands.h"
#include "protocols/ping.h"
#include "protocols/snmp.h"
#include "message-bus.h"
#include <fty/fty-log.h>
#include <fty/split.h>


namespace fty::job {

// =====================================================================================================================

/// Response wrapper
class ConfResponse : public BasicResponse<ConfResponse>
{
public:
    commands::mibs::Out mibs = FIELD("mibs");

public:
    using BasicResponse::BasicResponse;
    META(ConfResponse, mibs);

public:
    const commands::mibs::Out& data()
    {
        return mibs;
    }
};

// =====================================================================================================================

static bool sortMibs(const std::string& l, const std::string& r)
{
    // clang-format off
    static const std::vector<std::regex> snmpMibPriority = {
        std::regex("XUPS-MIB"),
        std::regex("MG-SNMP-UPS-MIB"),
        std::regex(".+")
    };
    // clang-format on

    auto index = [&](const std::string& mib) -> size_t {
        for (size_t i = 0; i < snmpMibPriority.size(); ++i) {
            if (std::regex_match(mib, snmpMibPriority[i])) {
                return i;
            }
        }
        return 999;
    };

    return index(l) < index(r);
}

// =====================================================================================================================

Configure::Configure(const Message& in, MessageBus& bus)
    : m_in(in)
    , m_bus(&bus)
{
}

void Configure::operator()()
{
    ConfResponse response;

    if (m_in.userData.empty()) {
        response.setError("Wrong input data");
        if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
            logError() << res.error();
        }
        return;
    }

    Expected<commands::mibs::In> cmd = m_in.userData.decode<commands::mibs::In>();
    if (!cmd) {
        response.setError("Wrong input data");
        if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
            logError() << res.error();
        }
        return;
    }

    if (!available(cmd->address)) {
        response.setError("Host is not available");
        if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
            logError() << res.error();
        }
        return;
    }

    auto info = readSnmp(cmd->address);
    if (!info) {
        logError() << info.error();
        response.setError(info.error());
        if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
            logError() << res.error();
        }
        return;
    }

    response.mibs = info->mibs;
    response.mibs.sort(sortMibs);

    logInfo() << Logger::nowhitespace() << "Configure: '" << info->name << "' mibs: [" << implode(response.mibs, ", ")
              << "]";

    response.status = Message::Status::Ok;
    if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
        logError() << res.error();
    }
}

// =====================================================================================================================

} // namespace fty::job
