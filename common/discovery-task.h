#pragma once
#include "commands.h"
#include "message-bus.h"
#include "message.h"
#include <fty/expected.h>
#include <fty/thread-pool.h>
#include <fty_log.h>

namespace fty::job {

// =====================================================================================================================

template <typename... Args>
[[maybe_unused]] static std::string formatString(const std::string& msg, const Args&... args)
{
    try {
        return fmt::format(msg, args...);
    } catch (const fmt::format_error&) {
        return msg;
    }
}

class Error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
    template <typename... Args>
    Error(const std::string& fmt, const Args&... args)
        : std::runtime_error(formatString(fmt, args...))
    {
    }
};

// =====================================================================================================================

/// Basic responce wrapper
template <typename T>
class Response : public pack::Node
{
public:
    pack::String                       error  = FIELD("error");
    pack::Enum<disco::Message::Status> status = FIELD("status");
    T                                  out    = FIELD("out");

public:
    using pack::Node::Node;
    META(Response, error, status, out);

public:
    void setError(const std::string& errMsg)
    {
        error  = errMsg;
        status = disco::Message::Status::Error;
    }

    operator disco::Message()
    {
        disco::Message msg;
        msg.meta.status = status;
        if (status == disco::Message::Status::Ok) {
            if (out.hasValue()) {
                msg.userData.setString(*pack::json::serialize(out));
            } else {
                if constexpr (std::is_base_of_v<pack::IList, T>) {
                    msg.userData.setString("[]");
                } else if constexpr (std::is_base_of_v<pack::IMap, T> || std::is_base_of_v<pack::INode, T>) {
                    msg.userData.setString("{}");
                } else {
                    msg.userData.setString("");
                }
            }
        } else {
            msg.userData.setString(error);
        }
        return msg;
    }
};

// =====================================================================================================================

template <typename T, typename InputT, typename ResponseT>
class Task : public fty::Task<T>
{
public:
    Task(const disco::Message& in, disco::MessageBus& bus)
        : m_in(in)
        , m_bus(&bus)
    {
    }

    void operator()() override
    {
        Response<ResponseT> response;
        try {
            if (m_in.userData.empty()) {
                throw Error("Wrong input data: payload is empty");
            }

            InputT cmd;
            if (auto parsedCmd = m_in.userData.decode<InputT>()) {
                cmd = *parsedCmd;
            } else {
                throw Error("Wrong input data: format of payload is incorrect");
            }

            if (auto it = dynamic_cast<T*>(this)) {
                it->run(cmd, response.out);
            } else {
                throw Error("Not a correct task");
            }

            response.status = disco::Message::Status::Ok;
            if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
                log_error(res.error().c_str());
            }
        } catch (const Error& err) {
            log_error("Error: %s", err.what());
            response.setError(err.what());
            if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
                log_error(res.error().c_str());
            }
        }
    }

protected:
    disco::Message     m_in;
    disco::MessageBus* m_bus;
};

} // namespace fty::job
