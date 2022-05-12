#pragma once
#include <fty/expected.h>

namespace fty {
class Process;
}

namespace fty::disco::impl::nut {

class Process
{
public:
    static const uint32_t WAIT_TIME_OUT_MS = 3 * 60 * 1000;  // Wait timeout of 3 mn

    Process(const std::string& protocol);
    ~Process();

    Expected<void>        init(const std::string& address, uint16_t port = 0);
    Expected<void>        setCredentialId(const std::string& credential);
    Expected<void>        setCredential(const std::string& userName, const std::string& password);
    Expected<void>        setCommunity(const std::string& community);
    Expected<void>        setTimeout(uint32_t milliseconds);
    Expected<void>        setMib(const std::string& mib);
    Expected<std::string> run() const;

private:
    Expected<std::string> findExecutable(const std::string& name) const;
    Expected<void> setupSnmp(const std::string& address, uint16_t port);
    Expected<void> setupXmlPdc(const std::string& address, uint16_t port);
    Expected<void> setupPowercom(const std::string& address, uint16_t port);

private:
    std::string                   m_protocol; // "nut_snmp", "nut_xml_pdc", "nut_powercom", ...
    std::string                   m_root;
    std::unique_ptr<fty::Process> m_process;
};

} // namespace fty::disco::impl::nut
