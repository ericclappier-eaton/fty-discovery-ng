#include "discovery-config.h"

namespace fty::disco {
std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Discovery::Type value)
{
    using Type = fty::disco::ConfigDiscovery::Discovery::Type;

    ss << [&]() {
        switch (value) {
            case Type::Local:
                return "localscan";
            case Type::Ip:
                return "ipscan";
            case Type::Multy:
                return "multiscan";
            case Type::Full:
                return "fullscan";
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
    if (strval == "localscan") {
        value = Type::Local;
    } else if (strval == "ipscan") {
        value = Type::Ip;
    } else if (strval == "multiscan") {
        value = Type::Multy;
    } else if (strval == "fullscan") {
        value = Type::Full;
    } else {
        value = Type::Unknown;
    }
    return ss;
}

} // namespace fty::disco
