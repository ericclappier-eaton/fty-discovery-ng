#include "commands.h"


namespace fty::disco::commands::scan {

std::ostream& operator<<(std::ostream& ss, In::Type value)
{
    using Type = In::Type;

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

std::istream& operator>>(std::istream& ss, In::Type& value)
{
    using Type = In::Type;

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

std::ostream& operator<<(std::ostream& ss, Status value)
{
    ss << [&]() {
        switch (value) {
            case Status::CancelledByUser:
            case Status::Terminated:
            case Status::InProgress:
            case Status::Unknown:
                return value;
        }
        return Status::Unknown;
    }();
    return ss;
}
std::istream& operator>>(std::istream& ss, Status& value)
{
    std::string strval;
    ss >> strval;

    value = static_cast<Status>(fty::convert<int>(strval));
    return ss;
}


} // namespace fty::disco::commands::scan