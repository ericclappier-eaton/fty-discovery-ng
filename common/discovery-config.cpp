#include "discovery-config.h"
#include <fstream>
#include <map>
#include <pack/serialization.h>

namespace fty::disco {

fty::Expected<void> ConfigDiscovery::save(const std::string& path)
{
    return pack::yaml::serializeFile(path, *this, pack::Option::WithDefaults);
}

fty::Expected<void> ConfigDiscovery::load(const std::string& path)
{
    return pack::yaml::deserializeFile(path, *this);
}

std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Discovery::Type value)
{
    using Type = ConfigDiscovery::Discovery::Type;

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
    using Type = ConfigDiscovery::Discovery::Type;

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
