#include "commands.h"

namespace fty::disco {

namespace commands::protocols {
    std::ostream& operator<<(std::ostream& ss, Return::Available value)
    {
        ss << [&]() {
            switch (value) {
                case Return::Available::Unknown:
                    return "unknown";
                case Return::Available::No:
                    return "no";
                case Return::Available::Maybe:
                    return "maybe";
                case Return::Available::Yes:
                    return "yes";
            }
            return "unknown";
        }();
        return ss;
    }

    std::istream& operator>>(std::istream& ss, Return::Available& value)
    {
        std::string strval;
        ss >> strval;
        if (strval == "unknown") {
            value = Return::Available::Unknown;
        } else if (strval == "no") {
            value = Return::Available::No;
        } else if (strval == "maybe") {
            value = Return::Available::Maybe;
        } else if (strval == "yes") {
            value = Return::Available::Yes;
        } else {
            value = Return::Available::Unknown;
        }
        return ss;
    }
} // namespace commands::protocols

namespace commands::scan::status {
    std::ostream& operator<<(std::ostream& ss, Out::Status value)
    {
        ss << [&]() {
            switch (value) {
                case Out::Status::CancelledByUser:
                    return "1";
                case Out::Status::Terminated:
                    return "2";
                case Out::Status::InProgess:
                    return "3";
                case Out::Status::Unknown:
                    return "0";
            }
            return "0";
        }();
        return ss;
    }

    std::istream& operator>>(std::istream& ss, Out::Status& value)
    {
        std::string strval;
        ss >> strval;
        if (strval == "1") {
            value = Out::Status::CancelledByUser;
        } else if (strval == "2") {
            value = Out::Status::Terminated;
        } else if (strval == "3") {
            value = Out::Status::InProgess;
        } else {
            value = Out::Status::Unknown;
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
            value = Status::Success ;
        } else if (strval == "failure") {
            value = Status::Failure;
        } else {
            value = Status::Unknown;
        }
        return ss;
    }
} // namespace commands::scan
} // namespace fty::disco
