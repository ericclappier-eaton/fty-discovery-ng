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
            case Type::Powercom:
                return "nut_powercom";
            case Type::XmlPdc:
                return "nut_xml_pdc";
            case Type::Snmp:
                return "nut_snmp";
            case Type::Unknown:
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
        value = Type::Powercom;
    } else if (strval == "nut_xml_pdc") {
        value = Type::XmlPdc;
    } else if (strval == "nut_snmp") {
        value = Type::Snmp;
    } else {
        value = Type::Unknown;
    }
    return ss;
}

std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Discovery::Type value)
{
    using Type = ConfigDiscovery::Discovery::Type;

    ss << [&]() {
        switch (value) {
            case Type::Local:
                return "local_scan";
            case Type::Ip:
                return "ip_scan";
            case Type::Multi:
                return "multi_scan";
            case Type::Full:
                return "full_scan";
            case Type::Unknown:
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
    if (strval == "local_scan") {
        value = Type::Local;
    } else if (strval == "ip_scan") {
        value = Type::Ip;
    } else if (strval == "multi_scan") {
        value = Type::Multi;
    } else if (strval == "full_scan") {
        value = Type::Full;
    } else {
        value = Type::Unknown;
    }
    return ss;
}

std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::DefaultValuesAux::Priority value)
{
    using Priority = ConfigDiscovery::DefaultValuesAux::Priority;

    ss << [&]() {
        switch (value) {
            case Priority::P1:
                return "P1";
            case Priority::P2:
                return "P2";
            case Priority::P3:
                return "P3";
            case Priority::P4:
                return "P4";
            case Priority::P5:
                return "P5";
        }
        return "P1";
    }();
    return ss;
}

std::istream& operator>>(std::istream& ss, ConfigDiscovery::DefaultValuesAux::Priority& value)
{
    using Priority = ConfigDiscovery::DefaultValuesAux::Priority;

    std::string strval;
    ss >> strval;
    if (strval == "P1") {
        value = Priority::P1;
    } else if (strval == "P2") {
        value = Priority::P2;
    } else if (strval == "P3") {
        value = Priority::P3;
    } else if (strval == "P4") {
        value = Priority::P4;
    } else if (strval == "P5") {
        value = Priority::P5;
    } else {
        value = Priority::P1;
    }
    return ss;
}

} // namespace fty::disco
