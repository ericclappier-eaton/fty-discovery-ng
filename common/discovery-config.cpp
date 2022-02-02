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

std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Protocol::Type value){
    using Type = ConfigDiscovery::Protocol::Type;

    ss << [&]() {
        switch (value) {
            case Type::POWERCOM:
                return "nut_powercom";
            case Type::XML_PDC:
                return "nut_xml_pdc";
            case Type::SNMP:
                return "nut_snmp";
            case Type::UNKNOWN:
                return "unknown";
        }
        return "unknown";
    }();
    return ss;
}
std::istream& operator>>(std::istream& ss, ConfigDiscovery::Protocol::Type& value){
    using Type = ConfigDiscovery::Protocol::Type;

    std::string strval;
    ss >> strval;
    if (strval == "nut_powercom") {
        value = Type::POWERCOM;
    } else if (strval == "nut_xml_pdc") {
        value = Type::XML_PDC;
    } else if (strval == "nut_snmp") {
        value = Type::SNMP;
    } else {
        value = Type::UNKNOWN;
    }
    return ss;
}

std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Discovery::Type value)
{
    using Type = ConfigDiscovery::Discovery::Type;

    ss << [&]() {
        switch (value) {
            case Type::LOCAL:
                return "localscan";
            case Type::IP:
                return "ipscan";
            case Type::MULTI:
                return "multiscan";
            case Type::FULL:
                return "fullscan";
            case Type::UNKNOWN:
                return "unknown";
        }
        return "unknown";
    }();
    return ss;
}

std::istream& operator>>(std::istream& ss, ConfigDiscovery::Discovery::Type& value)
{
    using Type = ConfigDiscovery::Discovery::Type;

    std::string strval;
    ss >> strval;
    if (strval == "localscan") {
        value = Type::LOCAL;
    } else if (strval == "ipscan") {
        value = Type::IP;
    } else if (strval == "multiscan") {
        value = Type::MULTI;
    } else if (strval == "fullscan") {
        value = Type::FULL;
    } else {
        value = Type::UNKNOWN;
    }
    return ss;
}



} // namespace fty::disco
