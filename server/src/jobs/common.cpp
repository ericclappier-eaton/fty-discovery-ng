/*  =========================================================================
    common.cpp - Jobs common stuff

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

#include "common.h"
#include "protocols/snmp.h"
#include "src/config.h"
#include <regex>
#include <set>

namespace fty::job {

Expected<BasicInfo> readSnmp(const std::string& ipAddress)
{
    auto session = protocol::Snmp::instance().session(ipAddress);
    if (auto res = session->open(); !res) {
        return unexpected(res.error());
    }

    auto name = session->read("SNMPv2-MIB::sysDescr.0");
    if (!name) {
        return unexpected(name.error());
    }

    BasicInfo info;
    info.name = *name;
    info.type = BasicInfo::Type::Snmp;

    std::set<std::string> mibs;

    if (fty::Config::instance().tryAll) {
        auto res = session->walk([&](const std::string& mib) {
            if (filterMib(mib)) {
                size_t pos;
                if (pos = mib.find("::"); pos != std::string::npos) {
                    mibs.insert(mib.substr(0, pos));
                }
            }
        });
        if (!res) {
            return unexpected(res.error());
        }
    } else {
        for (const std::string& mib : knownMibs()) {
            if (auto val = session->read(mib); !val) {
                continue;
            }

            size_t pos;
            if (pos = mib.find("::"); pos != std::string::npos) {
                mibs.insert(mib.substr(0, pos));
            }
        }
    }

    info.mibs.setValue(std::vector<std::string>(mibs.begin(), mibs.end()));

    return std::move(info);
}

bool filterMib(const std::string& mib)
{
    static std::regex rex("^(IP-MIB|DISMAN-EVENT-MIB|RFC1213-MIB|SNMP-|TCP-MIB|UDP-MIB).*");
    return !std::regex_match(mib, rex);
}

const std::vector<std::string>& knownMibs()
{
    static std::vector<std::string> mibs = {
        "PowerNet-MIB::atsIdentModelNumber.0",     // apc_ats
        "PowerNet-MIB::upsBasicIdentModel.0",      // apcc
        "PowerNet-MIB::sPDUIdentModelNumber.0",    // apc_pdu
        "Baytech-MIB-503-1::sBTAModulesRPCType.1", // baytech
        "BESTPOWER-MIB::upsIdentModel.0",          // bestpower
        "CPQPOWER-MIB::upsIdentManufacturer.0",    // cpqpower
        "CPS-MIB::upsBaseIdentModel.0",            // cyberpower
        "DeltaUPS-MIB::upsv4",                     // delta_ups
        "EATON-OIDS::sts",                         // eaton_ats30
        "EATON-GENESIS-II-MIB::productTitle.0",    // aphel_genesisII
        "EATON-EPDU-MIB::productName.0",           // eaton_epdu
        "EATON-EPDU-PU-SW-MIB::pulizzi.1",         // pulizzi_switched1
        "CPQPOWER-MIB::pdu2Model.0",               // hpe_epdu
        "TRIPPUPS1-MIB::trippUPS1",                // tripplite
        "UPS-MIB::upsIdentManufacturer.0",         // ietf
        "MG-SNMP-UPS-MIB::upsmgIdentFamilyName.0", // mge
        "XUPS-MIB::xupsIdentModel.0",              // pw
        "EATON-OIDS::eatonPowerChainDevice",       // pxgx_ups
        "XPPC-MIB::ppc"                            // xppx
    };
    return mibs;
}

} // namespace fty::job
