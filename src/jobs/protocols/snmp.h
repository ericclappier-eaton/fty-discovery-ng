#pragma once

#include <memory>
#include <fty/expected.h>

namespace fty::protocol {

class Snmp
{
public:
    Snmp();
    ~Snmp();

    Expected<bool> discover(const std::string& addr) const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace fty::protocol
