#pragma once
#include <fty/expected.h>

namespace fty {
class Process;
}

namespace fty::impl::nut {

class Process
{
public:
    Process(const std::string& protocol);
    ~Process();

    Expected<void>        init(const std::string& address, uint16_t port = 0);
    Expected<void>        setCredentialId(const std::string& credential);
    Expected<void>        setCommunity(const std::string& community);
    Expected<void>        setTimeout(uint milliseconds);
    Expected<void>        setMib(const std::string& mib);
    Expected<std::string> run() const;

private:
    Expected<std::string> findExecutable(const std::string& name) const;

private:
    std::string                   m_protocol;
    std::string                   m_root;
    std::unique_ptr<fty::Process> m_process;
};

} // namespace fty::protocol::nut
