#include "commands.h"

namespace fty::disco {

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
