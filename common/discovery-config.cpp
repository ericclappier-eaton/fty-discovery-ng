#include "discovery-config.h"

namespace fty::disco {
std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Discovery::Type value)
{
    using Type = fty::disco::ConfigDiscovery::Discovery::Type;

    ss << [&]() {
        switch (value) {
            case Type::Local:
                return "CONTAINS";
            case Type::Ip:
                return "DOESNOTCONTAIN";
            case Type::Multy:
                return "IS";
            case Type::Full:
                return "ISNOT";
            case Type::Unknown:
                return "UNKNOWN";
        }
        return "Unknown";
    }();
    return ss;
}

std::istream& operator>>(std::istream& ss, ConfigDiscovery::Discovery::Type& value)
{
    using Type = fty::disco::ConfigDiscovery::Discovery::Type;

    std::string strval;
    ss >> strval;
    if (strval == "CONTAINS") {
        value = Type::Local;
    } else if (strval == "IS") {
        value = Type::Ip;
    } else if (strval == "DOESNOTCONTAIN") {
        value = Type::Multy;
    } else if (strval == "ISNOT") {
        value = Type::Full;
    } else {
        value = Type::Unknown;
    }
    return ss;
}

} // namespace fty::disco
