/*  =========================================================================
    snmp.cpp - SNMP simple implementation

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

#include "snmp.h"
// Config should be firt
#include <net-snmp/net-snmp-config.h>
// Snmp stuff
#include <net-snmp/mib_api.h>
#include <net-snmp/session_api.h>
#include <net-snmp/snmpv3_api.h>
// Other
#include <fty/expected.h>
#include <fty_security_wallet.h>
#include <iostream>
#include <regex>
#include <set>

namespace fty::protocol {


class Snmp::Session::Impl
{
public:
    Impl(const std::string& addr, uint16_t port)
        : m_addr(addr + ":" + std::to_string(port))
    {
    }

    virtual ~Impl()
    {
        if (m_handle) {
            snmp_sess_close(m_handle);
        }
    }

    Expected<void> open()
    {
        m_handle = snmp_sess_open(&m_sess);
        if (!m_handle) {
            log_error(snmp_api_errstring(snmp_errno));
            return unexpected(snmp_api_errstring(snmp_errno));
        }
        return {};
    }

    Expected<std::string> read(const std::string& stroid)
    {
        oid    name[MAX_OID_LEN];
        size_t nameLen = MAX_OID_LEN;

        if (!snmp_parse_oid(stroid.c_str(), name, &nameLen)) {
            return unexpected("Cannot parse OID '{}'", stroid);
        }

        netsnmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GET);
        snmp_add_null_var(pdu, name, nameLen);

        netsnmp_pdu* response = nullptr;
        int          status   = snmp_sess_synch_response(m_handle, pdu, &response);
        std::unique_ptr<netsnmp_pdu, std::function<void(netsnmp_pdu*)>> rptr(response, [](netsnmp_pdu* p) {
            snmp_free_pdu(p);
        });

        if (status == STAT_SUCCESS) {
            if (response->errstat == SNMP_ERR_NOERROR) {
                if (response->variables->val_len > 0) {
                    return readVal(response->variables);
                } else {
                    unexpected("Wrong value type");
                }
            } else {
                return unexpected(snmp_errstring(int(response->errstat)));
            }
        }
        return unexpected(snmp_api_errstring(snmp_errno));
    }

    Expected<void> walk(std::function<void(const std::string&)>&& func)
    {
        oid    name[MAX_OID_LEN];
        size_t nameLen = MAX_OID_LEN;

        bool running = true;
        snmp_parse_oid(".1.3.6.1.2.1", name, &nameLen);
        if (!snmp_parse_oid(".1.3.6.1.2.1", name, &nameLen)) {
            return unexpected("Cannot parse root OID '.1.3.6.1.2.1'");
        }

        std::array<char, 255> buff;

        while (running) {
            netsnmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
            snmp_add_null_var(pdu, name, nameLen);

            netsnmp_pdu* response = nullptr;
            int          status   = snmp_sess_synch_response(m_handle, pdu, &response);
            std::unique_ptr<netsnmp_pdu, std::function<void(netsnmp_pdu*)>> rptr(response, [](netsnmp_pdu* p) {
                snmp_free_pdu(p);
            });

            if (status == STAT_ERROR) {
                return unexpected(snmp_errstring(snmp_errno));
            }
            if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
                for (auto vars = response->variables; vars; vars = vars->next_variable) {
                    snprint_objid(buff.data(), buff.size(), vars->name, vars->name_length);
                    func(buff.data());
                    if ((vars->type != SNMP_ENDOFMIBVIEW) && (vars->type != SNMP_NOSUCHOBJECT) &&
                        (vars->type != SNMP_NOSUCHINSTANCE)) {
                        memmove(name, vars->name, vars->name_length * sizeof(oid));
                        nameLen = vars->name_length;
                    } else {
                        break;
                    }
                }
            } else {
                break;
            }
        }
        return {};
    }

private:
    Expected<std::string> readVal(const netsnmp_variable_list* lst)
    {
        switch (lst->type) {
            case ASN_BOOLEAN:
            case ASN_INTEGER:
            case ASN_COUNTER:
            case ASN_GAUGE:
            case ASN_TIMETICKS:
            case ASN_UINTEGER:
                return convert<std::string>(int64_t(lst->val.integer));
            case ASN_BIT_STR:
            case ASN_OCTET_STR:
            case ASN_OPAQUE:
                return std::string(reinterpret_cast<const char*>(lst->val.string), lst->val_len);
            case ASN_IPADDRESS:
                return fmt::format("{:d}.{:d}.{:d}.{:d}", lst->val.string[0], lst->val.string[1], lst->val.string[2],
                    lst->val.string[3]);
            case ASN_OBJECT_ID:
                return readObjName(lst);
        }
        return unexpected("Unsupported type or null value");
    }

    Expected<std::string> readObjName(const netsnmp_variable_list* lst)
    {
        std::array<char, 255> buff;
        snprint_objid(buff.data(), buff.size(), lst->val.objid, lst->val_len / sizeof(oid));
        return std::string(buff.data());
    }

private:
    friend class SessionCommunity;
    friend class SessionWallet;

    void*           m_handle = nullptr;
    netsnmp_session m_sess;
    std::string     m_addr;
};

// =====================================================================================================================

class SessionCommunity : public Snmp::Session
{
public:
    SessionCommunity(const std::string& addr, uint16_t port, const std::string& community, uint32_t timeout)
        : Snmp::Session(addr, port)
    {
        snmp_sess_init(&m_impl->m_sess);

        m_impl->m_sess.peername      = const_cast<char*>(m_impl->m_addr.c_str());
        m_impl->m_sess.version       = SNMP_VERSION_1;
        m_impl->m_sess.retries       = 1;
        m_impl->m_sess.timeout       = timeout * 1000 * 1000;
        m_impl->m_sess.community     = const_cast<u_char*>(reinterpret_cast<const u_char*>(community.c_str()));
        m_impl->m_sess.community_len = strlen(reinterpret_cast<const char*>(m_impl->m_sess.community));
    }
};

class SessionWallet : public Snmp::Session
{
public:
    SessionWallet(const std::string& addr, uint16_t port, const std::string& securityId, uint32_t timeout)
        : Snmp::Session(addr, port)
    {
        snmp_sess_init(&m_impl->m_sess);
        m_impl->m_sess.peername = const_cast<char*>(m_impl->m_addr.c_str());
        m_impl->m_sess.retries  = 1;
        m_impl->m_sess.timeout  = timeout * 1000 * 1000;

        fty::SocketSyncClient secwSyncClient("/run/fty-security-wallet/secw.socket");
        auto                  client  = secw::ConsumerAccessor(secwSyncClient);
        auto                  secCred = client.getDocumentWithPrivateData("default", securityId);

        if (auto credV3 = secw::Snmpv3::tryToCast(secCred)) {
            auto level = [](secw::Snmpv3SecurityLevel lvl) -> Expected<int> {
                switch (lvl) {
                    case secw::NO_AUTH_NO_PRIV:
                        return SNMP_SEC_LEVEL_NOAUTH;
                    case secw::AUTH_NO_PRIV:
                        return SNMP_SEC_LEVEL_AUTHNOPRIV;
                    case secw::AUTH_PRIV:
                        return SNMP_SEC_LEVEL_AUTHPRIV;
                    case secw::MAX_SECURITY_LEVEL:
                        return unexpected("No privs defined");
                }
                return unexpected("No privs defined");
            };

            auto authProt = [](secw::Snmpv3AuthProtocol proc) -> Expected<oid*> {
                switch (proc) {
                    case secw::MD5:
                        return usmHMACMD5AuthProtocol;
                    case secw::SHA:
                        return usmHMACSHA1AuthProtocol;
                    case secw::MAX_AUTH_PROTOCOL:
                        return unexpected("Wrong protocol");
                }
                return unexpected("Wrong protocol");
            };

            auto authPriv = [](secw::Snmpv3PrivProtocol proc) -> Expected<oid*> {
                switch (proc) {
                    case secw::DES:
                        return usmDESPrivProtocol;
                    case secw::AES:
                        return usmAESPrivProtocol;
                    case secw::MAX_PRIV_PROTOCOL:
                        return unexpected("Wrong protocol");
                }
                return unexpected("Wrong protocol");
            };

            m_impl->m_sess.version = SNMP_VERSION_3;

            m_impl->m_sess.securityName    = strdup(credV3->getSecurityName().c_str());
            m_impl->m_sess.securityNameLen = strlen(m_impl->m_sess.securityName);

            if (auto lvl = level(credV3->getSecurityLevel())) {
                m_impl->m_sess.securityLevel = *lvl;
            }

            if (auto prot = authProt(credV3->getAuthProtocol())) {
                m_impl->m_sess.securityAuthProto    = *prot;
                m_impl->m_sess.securityAuthProtoLen = sizeof(usmHMACMD5AuthProtocol) / sizeof(oid);
                m_impl->m_sess.securityAuthKeyLen   = USM_AUTH_KU_LEN;
            }

            if (auto prot = authPriv(credV3->getPrivProtocol())) {
                m_impl->m_sess.securityPrivProto    = *prot;
                m_impl->m_sess.securityPrivProtoLen = sizeof(usmDESPrivProtocol) / sizeof(oid);
                m_impl->m_sess.securityPrivKeyLen   = USM_AUTH_KU_LEN;
            }

            if (generate_Ku(m_impl->m_sess.securityAuthProto, u_int(m_impl->m_sess.securityAuthProtoLen),
                    const_cast<u_char*>(reinterpret_cast<const u_char*>(credV3->getAuthPassword().c_str())),
                    u_int(credV3->getAuthPassword().size()), m_impl->m_sess.securityAuthKey,
                    &m_impl->m_sess.securityAuthKeyLen) != SNMPERR_SUCCESS) {
                log_error("Error generating Ku from authentication pass phrase.");
            }
            if (generate_Ku(m_impl->m_sess.securityPrivProto, u_int(m_impl->m_sess.securityPrivProtoLen),
                    const_cast<u_char*>(reinterpret_cast<const u_char*>(credV3->getPrivPassword().c_str())),
                    u_int(credV3->getPrivPassword().size()), m_impl->m_sess.securityPrivKey,
                    &m_impl->m_sess.securityPrivKeyLen) != SNMPERR_SUCCESS) {
                log_error("Error generating Ku from authentication pass phrase.");
            }
        } else if (auto credV1 = secw::Snmpv1::tryToCast(secCred)) {
            m_impl->m_sess.version = SNMP_VERSION_1;
            m_impl->m_sess.community =
                const_cast<u_char*>(reinterpret_cast<const u_char*>(strdup(credV1->getCommunityName().c_str())));
            m_impl->m_sess.community_len = strlen(reinterpret_cast<const char*>(m_impl->m_sess.community));
        }
    }
};

// =====================================================================================================================

Snmp::Snmp()
{
}

Snmp::~Snmp()
{
    shutdown_mib();
}

Snmp& Snmp::instance()
{
    static Snmp inst;
    return inst;
}

void Snmp::init(const std::string& mibsPath)
{
    setenv("MIBS", "ALL", 1);
    netsnmp_get_mib_directory();
    netsnmp_set_mib_directory(mibsPath.c_str());
    add_mibdir(mibsPath.c_str());

    netsnmp_init_mib();
    init_snmp("fty-discovery");

    read_all_mibs();
}

Snmp::SessionPtr Snmp::sessionByCommunity(const std::string& address, u_int16_t port, const std::string& community, uint32_t timeout)
{
    return std::shared_ptr<Session>(new SessionCommunity(address, port, community, timeout));
}

Snmp::SessionPtr Snmp::sessionByWallet(const std::string& address, u_int16_t port, const std::string& securityId, uint32_t timeout)
{
    return std::shared_ptr<Session>(new SessionWallet(address, port, securityId, timeout));
}

// =====================================================================================================================

Snmp::Session::Session(const std::string& addr, uint16_t port)
    : m_impl(new Impl(addr, port))
{
}

Expected<void> Snmp::Session::open()
{
    return m_impl->open();
}

Expected<std::string> Snmp::Session::read(const std::string& oid) const
{
    return m_impl->read(oid);
}

Expected<void> Snmp::Session::walk(std::function<void(const std::string&)>&& func) const
{
    return m_impl->walk(std::move(func));
}

// =====================================================================================================================

} // namespace fty::protocol
