#pragma once
#include <fty/expected.h>

namespace fty::protocol {

class Xml
{
public:
    Xml();

    Expected<bool> discover(const std::string& addr) const;
};

} // namespace fty::protocol
