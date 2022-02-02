#include "commands.h"

namespace fty::disco {

namespace commands::scan::status {
    std::ostream& operator<<(std::ostream& ss, Status value)
    {
        ss << [&]() {
            switch (value) {
                case Status::CANCELLED_BY_USER:
                    return "1";
                case Status::TERMINATED:
                    return "2";
                case Status::IN_PROGRESS:
                    return "3";
                case Status::UNKNOWN:
                    return "0";
            }
            return "0";
        }();
        return ss;
    }

    std::istream& operator>>(std::istream& ss, Status& value)
    {
        std::string strval;
        ss >> strval;
        if (strval == "1") {
            value = Status::CANCELLED_BY_USER;
        } else if (strval == "2") {
            value = Status::TERMINATED;
        } else if (strval == "3") {
            value = Status::IN_PROGRESS;
        } else {
            value = Status::UNKNOWN;
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
                case Status::SUCCESS:
                    return "success";
                case Status::FAILURE:
                    return "failure";
                case Status::UNKNOWN:
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
            value = Status::SUCCESS ;
        } else if (strval == "failure") {
            value = Status::FAILURE;
        } else {
            value = Status::UNKNOWN;
        }
        return ss;
    }
} // namespace commands::scan
} // namespace fty::disco
