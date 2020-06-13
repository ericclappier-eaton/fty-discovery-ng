#include "snmp.h"
#include <fty/expected.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/library/snmp_api.h>

namespace fty::protocol {

class Session
{
public:
    Session(const std::string& addr)
        : m_addr(addr)
    {
        snmp_sess_init(&m_sess);

        m_sess.peername = const_cast<char*>(addr.c_str());
        m_sess.version  = SNMP_VERSION_1;
        m_sess.retries  = 0;
        m_sess.timeout  = 100 * 1000;
    }

    ~Session()
    {
        if (m_handle) {
            snmp_sess_close(m_handle);
        }
    }

    bool open()
    {
        m_handle = snmp_sess_open(&m_sess);
        if (!m_handle) {
            return false;
        }
        return true;
    }

private:
    void*           m_handle = nullptr;
    netsnmp_session m_sess;
    std::string     m_addr;
};

class Snmp::Impl
{
public:
    Impl()
    {
        init_snmp("fty-discovery");
    }

    Expected<bool> discover(const std::string& addr) const
    {
        Session sess(addr);
        return sess.open();
    }
};

// =====================================================================================================================

Snmp::Snmp()
    : m_impl(new Impl)
{
}

Snmp::~Snmp()
{
}

Expected<bool> Snmp::discover(const std::string& addr) const
{
    return m_impl->discover(addr);
}

// =====================================================================================================================

} // namespace fty::protocol
