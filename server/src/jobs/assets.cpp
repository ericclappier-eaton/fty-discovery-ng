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

#include "assets.h"
#include "impl/nut.h"
#include "impl/ping.h"
#include "impl/mibs.h"

namespace fty::job {

void Assets::run(const commands::assets::In& in, commands::assets::Out& out)
{
    if (!available(in.address)) {
        throw Error("Host is not available: {}", in.address.value());
    }

    // Workaround to check if snmp is available
    if (in.protocol == "NUT_SNMP") {
        uint32_t port = in.port;
        if (!in.port.hasValue()) {
            port = 161;
        }

        protocol::MibsReader reader(in.address, uint16_t(port));

        if (in.settings.credentialId.hasValue()) {
            if (auto res = reader.setCredentialId(in.settings.credentialId); !res) {
                throw Error(res.error());
            }
        } else if (in.settings.community.hasValue()) {
            if (auto res = reader.setCommunity(in.settings.community); !res) {
                throw Error(res.error());
            }
        } else {
            throw Error("Credential or community must be set");
        }

        if (auto mibs = reader.read(); !mibs) {
            throw Error(mibs.error());
        }
    }

    if (auto res = protocol::properties(in)) {
        std::string str = *pack::json::serialize(*res);
        log_debug("output %s", str.c_str());

        out = *res;
    } else {
        throw Error(res.error());
    }
}

} // namespace fty::job
