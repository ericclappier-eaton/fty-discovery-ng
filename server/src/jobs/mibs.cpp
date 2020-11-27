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

#include "mibs.h"
#include "impl/mibs.h"
#include "impl/ping.h"
#include <fty/split.h>
#include <set>


namespace fty::job {

// =====================================================================================================================

static bool sortMibs(const std::string& l, const std::string& r)
{
    try {
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
    } catch (...) {
        return false;
    }
}

// =====================================================================================================================

void Mibs::run(const commands::mibs::In& in, commands::mibs::Out& out)
{
    if (!available(in.address)) {
        throw Error("Host is not available: {}", in.address.value());
    }

    impl::MibsReader reader(in.address, uint16_t(in.port.value()));

    if (in.credentialId.hasValue()) {
        if (auto res = reader.setCredentialId(in.credentialId); !res) {
            throw Error(res.error());
        }
    } else if (in.community.hasValue()) {
        if (auto res = reader.setCommunity(in.community); !res) {
            throw Error(res.error());
        }
    } else {
        throw Error("Credential or community must be set");
    }

    if (in.timeout.hasValue()) {
        reader.setTimeout(in.timeout);
    }

    std::string assetName;
    if (auto name = reader.readName()) {
        assetName = *name;
    } else {
        throw Error("Host is not available or SNMP is not supported. SNMP error: {}", name.error());
    }

    if (auto mibs = reader.read()) {
        out.setValue(std::vector<std::string>(mibs->begin(), mibs->end()));
        out.sort(sortMibs);
        log_info("Configure: '%s' mibs: [%s]", assetName.c_str(), implode(out, ", ").c_str());
    } else {
        throw Error("Host is not available or SNMP is not supported. SNMP error: {}", mibs.error());
    }
}

// =====================================================================================================================

} // namespace fty::job
