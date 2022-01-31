#include "commands.h"

namespace fty::disco {


namespace commands::protocols {
    std::ostream& operator<<(std::ostream& ss, In::Protocol::Type value){
        using Type = In::Protocol::Type;

        ss << [&]() {
            switch (value) {
                case Type::Powercom:
                    return "nut_powercom";
                case Type::XML_pdc:
                    return "nut_xml_pdc";
                case Type::SNMP:
                    return "nut_snmp";
                case Type::Unknown:
                    return "unknown";
            }
            return "Unknown";
        }();
        return ss;
    }
    std::istream& operator>>(std::istream& ss, In::Protocol::Type& value){
        using Type = In::Protocol::Type;

        std::string strval;
        ss >> strval;
        if (strval == "nut_powercom") {
            value = Type::Powercom;
        } else if (strval == "nut_xml_pdc") {
            value = Type::XML_pdc;
        } else if (strval == "nut_snmp") {
            value = Type::SNMP;
        } else {
            value = Type::Unknown;
        }
        return ss;
    }

} // namespace commands::protocols

namespace commands::scan::status {
    std::ostream& operator<<(std::ostream& ss, Out::Status value)
    {
        using Status = Out::Status;

        ss << [&]() {
            switch (value) {
                case Status::CancelledByUser:
                    return "1";
                case Status::Terminated:
                    return "2";
                case Status::InProgress:
                    return "3";
                case Status::Unknown:
                    return "0";
            }
            return "0";
        }();
        return ss;
    }

    std::istream& operator>>(std::istream& ss, Out::Status& value)
    {
        using Status = Out::Status;

        std::string strval;
        ss >> strval;
        if (strval == "1") {
            value = Status::CancelledByUser;
        } else if (strval == "2") {
            value = Status::Terminated;
        } else if (strval == "3") {
            value = Status::InProgress;
        } else {
            value = Status::Unknown;
        }
        return ss;
    }
} // namespace commands::scan::status

namespace commands::scan {
    std::ostream& operator<<(std::ostream& ss, Response::Status value)
    {
        using Status = Response::Status;

        ss << [&]() {
            switch (value) {
                case Status::Success:
                    return "success";
                case Status::Failure:
                    return "failure";
                case Status::Unknown:
                    return "unknown";
            }
            return "unknown";
        }();
        return ss;
    }
    std::istream& operator>>(std::istream& ss, Response::Status& value)
    {
        using Status = Response::Status;

        std::string strval;
        ss >> strval;
        if (strval == "success") {
            value = Status::Success;
        } else if (strval == "failure") {
            value = Status::Failure;
        } else {
            value = Status::Unknown;
        }
        return ss;
    }
} // namespace commands::scan
} // namespace fty::disco
