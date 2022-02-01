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

#include "snmp.h"
// Config should be first
#include <net-snmp/net-snmp-config.h>
// Snmp stuff
#include <net-snmp/mib_api.h>
#include <net-snmp/session_api.h>
#include <net-snmp/snmpv3_api.h>
// Other
#include <fty/expected.h>
#include <fty_common_socket_sync_client.h>
#include <fty_log.h>
#include <fty_security_wallet.h>
#include <array>

namespace fty::impl {

// =====================================================================================================================
// Wallet to snmp values conversion
// =====================================================================================================================

static Expected<int> level(secw::Snmpv3SecurityLevel lvl)
{
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
}

static Expected<oid*> authProt(secw::Snmpv3AuthProtocol proc)
{
    switch (proc) {
        case secw::MD5:
            return usmHMACMD5AuthProtocol;
        case secw::SHA:
            return usmHMACSHA1AuthProtocol;
        case secw::MAX_AUTH_PROTOCOL:
            return unexpected("Wrong protocol");
    }
    return unexpected("Wrong protocol");
}

static Expected<oid*> authPriv(secw::Snmpv3PrivProtocol proc)
{
    switch (proc) {
        case secw::DES:
            return usmDESPrivProtocol;
        case secw::AES:
            return usmAESPrivProtocol;
        case secw::MAX_PRIV_PROTOCOL:
            return unexpected("Wrong protocol");
    }
    return unexpected("Wrong protocol");
}

// =====================================================================================================================
// Session private implementation
// =====================================================================================================================

class snmp::Session::Impl
{
public:
    Impl(const std::string& addr, uint16_t port)
        : m_addr(addr + ":" + std::to_string(port))
    {
        memset(&m_sess, 0, sizeof(m_sess));
        snmp_sess_init(&m_sess);

        m_sess.peername = const_cast<char*>(m_addr.c_str());
        m_sess.retries  = 1;

        setTimeout(500); // default (msec)
    }

    virtual ~Impl()
    {
        close();

        // release m_sess strdup'ed members
        if (m_sess.community) { free(m_sess.community); m_sess.community = nullptr; }
        if (m_sess.securityName) { free(m_sess.securityName); m_sess.securityName = nullptr; }
    }

    Expected<void> setCommunity(const std::string& community)
    {
        m_sess.version = SNMP_VERSION_1;

        if (m_sess.community) { free(m_sess.community); }
        m_sess.community     = const_cast<u_char*>(reinterpret_cast<const u_char*>(strdup(community.c_str())));
        m_sess.community_len = strlen(reinterpret_cast<const char*>(m_sess.community));
        return {};
    }

    Expected<void> setTimeout(uint32_t milliseconds)
    {
        m_sess.timeout = long(milliseconds) * 1000;
        return {};
    }

    Expected<void> setCredentialId(const std::string& credId)
    {
        try {
            fty::SocketSyncClient secwSyncClient("/run/fty-security-wallet/secw.socket");
            auto                  client  = secw::ConsumerAccessor(secwSyncClient);
            auto                  secCred = client.getDocumentWithPrivateData("default", credId);

            if (auto credV3 = secw::Snmpv3::tryToCast(secCred)) {
                m_sess.version = SNMP_VERSION_3;

                if (m_sess.securityName) { free(m_sess.securityName); }
                m_sess.securityName    = strdup(credV3->getSecurityName().c_str());
                m_sess.securityNameLen = credV3->getSecurityName().size();

                if (auto lvl = level(credV3->getSecurityLevel())) {
                    m_sess.securityLevel = *lvl;
                }

                if (auto prot = authProt(credV3->getAuthProtocol())) {
                    m_sess.securityAuthProto  = *prot;
                    m_sess.securityAuthKeyLen = USM_AUTH_KU_LEN;
                    if (m_sess.securityAuthProto == usmHMACMD5AuthProtocol)
                        m_sess.securityAuthProtoLen = OID_LENGTH(usmHMACMD5AuthProtocol);
                    else
                        m_sess.securityAuthProtoLen = OID_LENGTH(usmHMACSHA1AuthProtocol);
                }

                if (auto prot = authPriv(credV3->getPrivProtocol())) {
                    m_sess.securityPrivProto  = *prot;
                    m_sess.securityPrivKeyLen = USM_PRIV_KU_LEN;
                    /* FIXME: see https://github.com/42ity/nut/blob/FTY/drivers/snmp-ups.c#L79 */
                    if (m_sess.securityPrivProto == usmDESPrivProtocol)
                        m_sess.securityPrivProtoLen = OID_LENGTH(usmDESPrivProtocol);
                    else
                        m_sess.securityPrivProtoLen = OID_LENGTH(usmAESPrivProtocol);
                }

                if (generate_Ku(m_sess.securityAuthProto, u_int(m_sess.securityAuthProtoLen),
                        const_cast<u_char*>(reinterpret_cast<const u_char*>(credV3->getAuthPassword().c_str())),
                        u_int(credV3->getAuthPassword().size()), m_sess.securityAuthKey,
                        &m_sess.securityAuthKeyLen) != SNMPERR_SUCCESS) {
                    log_error("Error generating Ku from authentication pass phrase.");
                }
                if (generate_Ku(m_sess.securityAuthProto, u_int(m_sess.securityAuthProtoLen),
                        const_cast<u_char*>(reinterpret_cast<const u_char*>(credV3->getPrivPassword().c_str())),
                        u_int(credV3->getPrivPassword().size()), m_sess.securityPrivKey,
                        &m_sess.securityPrivKeyLen) != SNMPERR_SUCCESS) {
                    log_error("Error generating Ku from privacy pass phrase.");
                }
            } else if (auto credV1 = secw::Snmpv1::tryToCast(secCred)) {
                setCommunity(credV1->getCommunityName());
                m_sess.version = SNMP_VERSION_1;
            }
        } catch (const secw::SecwException& err) {
            return unexpected(err.what());
        }
        return {};
    }

    void close()
    {
        if (m_handle) {
            snmp_sess_close(m_handle);
            m_handle = nullptr;
        }
    }

    Expected<void> open()
    {
        close(); // eg. reopen

        m_handle = snmp_sess_open(&m_sess);
        if (!m_handle) {
            log_error("Snmp error: %s", snmp_api_errstring(snmp_errno));
            return unexpected(snmp_api_errstring(snmp_errno));
        }
        if (!snmp_sess_session(m_handle)) {
            log_error("Snmp error: %s", snmp_api_errstring(snmp_errno));
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

        // create protocol data unit
        netsnmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GET);
        snmp_add_null_var(pdu, name, nameLen);

        netsnmp_pdu* response = nullptr;
        int          status   = snmp_sess_synch_response(m_handle, pdu, &response);
        std::unique_ptr<netsnmp_pdu, std::function<void(netsnmp_pdu*)>> delr(response, [](netsnmp_pdu* p) {
            if (p) { snmp_free_pdu(p); }
        });

        if (status == STAT_SUCCESS) {
            if (!response) {
                return unexpected("Unexpected null response");
            }
            else if (response->errstat == SNMP_ERR_NOERROR) {
                if (response->variables && (response->variables->val_len > 0)) {
                    return readVal(response->variables);
                } else {
                    return unexpected("Wrong value type");
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

        const char* rootOid = ".1.3.6.1.2.1";
        if (!snmp_parse_oid(rootOid, name, &nameLen)) {
            return unexpected("Cannot parse root OID '{}'", rootOid);
        }

        std::array<char, 255> buff; //NOTICE char VS. oid?

        while (true) {
            netsnmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
            snmp_add_null_var(pdu, name, nameLen);
            std::unique_ptr<netsnmp_pdu, std::function<void(netsnmp_pdu*)>> delp(pdu, [](netsnmp_pdu* p) {
                if (p) { snmp_free_pdu(p); }
            });

            netsnmp_pdu* response = nullptr;
            int          status   = snmp_sess_synch_response(m_handle, pdu, &response);
            std::unique_ptr<netsnmp_pdu, std::function<void(netsnmp_pdu*)>> delr(response, [](netsnmp_pdu* p) {
                if (p) { snmp_free_pdu(p); }
            });

            if (status == STAT_ERROR) {
                return unexpected(snmp_errstring(snmp_errno));
            }

            //NOTICE assume response is not NULL on success, else crash
            if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
                //NOTICE assume response->variables never empty, else the while loop becomes infinite
                for (auto vars = response->variables; vars; vars = vars->next_variable) {
                    snprint_objid(buff.data(), buff.size(), vars->name, vars->name_length);
                    func(buff.data());
                    if ((vars->type != SNMP_ENDOFMIBVIEW) && (vars->type != SNMP_NOSUCHOBJECT) &&
                        (vars->type != SNMP_NOSUCHINSTANCE)) {
                        memmove(name, vars->name, vars->name_length * sizeof(oid));
                        nameLen = vars->name_length;
                    } else {
                        break; //for
                    }
                }
            } else {
                break; //while
            }
        }
        return {};
    }

private:
    Expected<std::string> readVal(const netsnmp_variable_list* lst)
    {
        if (!lst) {
            return unexpected("insconsistent arg");
        }

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
            default:;
        }
        return unexpected("Unsupported type");
    }

    Expected<std::string> readObjName(const netsnmp_variable_list* lst)
    {
        std::array<char, 255> buff; //NOTICE char VS. oid?
        snprint_objid(buff.data(), buff.size(), lst->val.objid, lst->val_len / sizeof(oid));
        return std::string(buff.data());
    }

private:
    void*           m_handle{nullptr};
    netsnmp_session m_sess;
    std::string     m_addr;
};

// =====================================================================================================================
// Session implementation
// =====================================================================================================================

snmp::Session::Session(const std::string& addr, uint16_t port)
    : m_impl(new Impl(addr, port))
{
}

Expected<void> snmp::Session::open()
{
    return m_impl->open();
}

void snmp::Session::close()
{
    return m_impl->close();
}

Expected<std::string> snmp::Session::read(const std::string& oid) const
{
    return m_impl->read(oid);
}

Expected<void> snmp::Session::walk(std::function<void(const std::string&)>&& func) const
{
    return m_impl->walk(std::move(func));
}

Expected<void> snmp::Session::setCommunity(const std::string& community)
{
    return m_impl->setCommunity(community);
}

Expected<void> snmp::Session::setTimeout(uint32_t milliseconds)
{
    return m_impl->setTimeout(milliseconds);
}

Expected<void> snmp::Session::setCredentialId(const std::string& credId)
{
    return m_impl->setCredentialId(credId);
}

// =====================================================================================================================
// SNMP implementation
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
    logInfo("Snmp::init (mibsPath: {})", mibsPath);

    setenv("MIBS", "ALL", 1);
    netsnmp_get_mib_directory();
    netsnmp_set_mib_directory(mibsPath.c_str());
    add_mibdir(mibsPath.c_str());

    netsnmp_init_mib();
    init_snmp("fty-discovery-ng");

    read_all_mibs();
}

snmp::SessionPtr Snmp::session(const std::string& address, uint16_t port)
{
    return std::shared_ptr<snmp::Session>(new snmp::Session(address, port));
}

} // namespace fty::impl
