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
#include "snmp.h"
#include "src/config.h"
#include <regex>
#include <fty_log.h>


namespace fty::protocol {

// =====================================================================================================================

bool filterMib(const std::string& mib)
{
    static std::regex rex("^(IP-MIB|DISMAN-EVENT-MIB|RFC1213-MIB|SNMP-|TCP-MIB|UDP-MIB).*");
    return !std::regex_match(mib, rex);
}

static const std::vector<std::string>& knownMibs()
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

bool isSnmpSupported(const std::string& mib)
{
    return !mapMibToLegacy(mib).empty();
}

std::string mapMibToLegacy(const std::string& mib)
{
    // clang-format off
    static std::map<std::string, std::string> mibs = {
        {"PowerNet-MIB::automaticXferSwitch",   "apc_ats"},             // apc_ats
        {"PowerNet-MIB::masterSwitchMSP",       "apc_pdu"},             // apc_pdu_rpdu
        {"PowerNet-MIB::masterSwitchrPDU",      "apc_pdu"},             // apc_pdu_rpdu2
        {"PowerNet-MIB::masterSwitchrPDU2",     "apc_pdu"},             // apc_pdu_msp
        {"PowerNet-MIB::smartUPS2200",          "apc_ats"},             // apc
        {"PowerNet-MIB::smartUPS3000",          "apc_ats"},             // apc
        {"PowerNet-MIB::smartUPS2",             "apc_ats"},             // apc
        {"PowerNet-MIB::smartUPS450",           "apc_ats"},             // apc
        {"PowerNet-MIB::smartUPS700",           "apc_ats"},             // apc
        {"Baytech-MIB-503-1::baytech",          "baytech"},             // baytech
        {"BESTPOWER-MIB::bestPower",            "bestpower"},           // bestpower
        {"CPQPOWER-MIB::ups",                   "cpqpower"},            // compaq
        {"CPS-MIB::cps",                        "cyberpower"},          // cyberpower
        {"DeltaUPS-MIB::upsv4",                 "delta_ups"},           // delta_ups
        {"MG-SNMP-UPS-MIB::upsmg",              "eaton_ats16"},         // eaton_ats16, legacy
        {"EATON-ATS2-MIB::ats2",                "eaton_ats16_g2"},      // eaton_ats16, g2
        {"EATON-OIDS::sts",                     "eaton_ats30"},         // eaton_ats30
        {"EATON-GENESIS-II-MIB::eaton",         "aphel_genesisII"},     // aphel_genesisII
        {"EATON-OIDS::pduAgent.6",              "aphel_revelation"},    // aphel_revelation
        {"EATON-EPDU-MIB::eatonEpdu",           "eaton_epdu"},          // eaton_marlin
        {"PM-MIB::pm3024",                      "emerson_avocent_pdu"}, // emerson_avocent_pdu
        {"EATON-EPDU-PU-SW-MIB::pulizzi",       "pulizzi_switched1"},   // pulizzi_switched1
        {"CPQPOWER-MIB::pdu2",                  "hpe_epdu"},            // hpe_pdu
        {"NET-SNMP-TC::linux",                  "huawei"},              // huawei
        {"MG-SNMP-UPS-MIB::upsmg",              "mge"},                 // mge
        {"EATON-OIDS::xupsMIB",                 "pw"},                  // powerware
        {"EATON-OIDS::eatonPowerChainDevice",   "pxgx_ups"},            // pxgx_ups
        {"PDU2-MIB::raritan",                   "raritan"},             // raritan
        {"PDU2-MIB::pdu2",                      "raritan_px2"},         // raritan_px2
        {"XPPC-MIB::ppc",                       "xppc"},                // xppc
        {"SOCOMECUPS-MIB::netvision",           "apc_ats"},             // netvision
        {"TRIPPUPS1-MIB::trippUPS1",            "tripplite"},           // tripplite_ietf
        {"UPS-MIB::upsMIB",                     "ietf"},                // ietf
    };
    // clang-format on
    auto it = mibs.find(mib);
    return it != mibs.end() ? it->second : "";
}

// =====================================================================================================================

MibsReader::MibsReader(const std::string& address, uint16_t port)
    : m_session(Snmp::instance().session(address, port))
{
}

Expected<void> MibsReader::setCredentialId(const std::string& credentialId)
{
    return m_session->setCredentialId(credentialId);
}

Expected<void> MibsReader::setCommunity(const std::string& community)
{
    return m_session->setCommunity(community);
}

Expected<void> MibsReader::setTimeout(uint miliseconds)
{
    return m_session->setTimeout(miliseconds);
}

Expected<MibsReader::MibList> MibsReader::read() const
{
    if (!m_isOpen) {
        if (auto res = m_session->open(); !res) {
            return unexpected(res.error());
        }
        m_isOpen = true;
    }

    MibList mibs;

    auto oid = m_session->read("RFC1213-MIB::sysObjectID.0");
    if (oid) {
        size_t pos;
        if (pos = oid->find("."); pos != std::string::npos) {
            mibs.insert(oid->substr(0, pos));
        } else {
            mibs.insert(*oid);
        }
    } else {
        if (fty::Config::instance().tryAll) {
            auto res = m_session->walk([&](const std::string& mib) {
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
                if (auto val = m_session->read(mib); !val) {
                    continue;
                }

                size_t pos;
                if (pos = mib.find("::"); pos != std::string::npos) {
                    mibs.insert(mib.substr(0, pos));
                }
            }
        }
        if (mibs.empty()) {
            return unexpected("Cannot fetch mibs from endpoint. Host is not available or SNMP is not supported.");
        }
    }

    return std::move(mibs);
}

Expected<std::string> MibsReader::readName() const
{
    if (!m_isOpen) {
        if (auto res = m_session->open(); !res) {
            return unexpected(res.error());
        }
        m_isOpen = true;
    }
    auto name = m_session->read("SNMPv2-MIB::sysDescr.0");
    if (!name) {
        return unexpected(name.error());
    }
    return *name;
}

// =====================================================================================================================

} // namespace fty::protocol
