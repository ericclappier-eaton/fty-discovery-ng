#include "commands.h"

namespace fty::disco {

namespace commands::scan::start {
    std::ostream& operator<<(std::ostream& ss, In::Type value)
    {
        using Type = In::Type;

        ss << [&]() {
            switch (value) {
                case Type::Local:
                    return "localscan";
                case Type::Ip:
                    return "ipscan";
                case Type::Multi:
                    return "multiscan";
                case Type::Full:
                    return "fullscan";
                case Type::Unknown:
                    return "unknown";
            }
            return "unknown";
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
            value = Type::Multi;
        } else if (strval == "fullscan") {
            value = Type::Full;
        } else {
            value = Type::Unknown;
        }
        return ss;
    }
} // namespace commands::scan::start

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
