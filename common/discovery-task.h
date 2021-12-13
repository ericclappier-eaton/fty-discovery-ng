#pragma once
#include "commands.h"
#include "message-bus.h"
#include "message.h"
#include <fty/expected.h>
#include <fty/thread-pool.h>
#include <fty_log.h>

namespace fty::disco::job {

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

    Task() = default;

    inline void operator()() override
    {
        Response<ResponseT> response;
        try {

            /* InputT cmd;
            if (auto parsedCmd = m_in.userData.decode<InputT>()) {
                cmd = *parsedCmd;
            } else {
                throw Error("Wrong input data: format of payload is incorrect");
            }

            if (auto it = dynamic_cast<T*>(this)) {
                it->run(cmd, response.out);
            } else {
                throw Error("Not a correct task");
            } */
            if (auto it = dynamic_cast<T*>(this)) {
                if constexpr (!std::is_same<InputT, void>::value) {
                    if (m_in.userData.empty()) {
                        throw Error("Wrong input data: payload is empty");
                    }
                    InputT cmd;
                    if (auto parsedCmd = m_in.userData.decode<InputT>()) {
                        cmd = *parsedCmd;
                    } else {
                        throw Error("Wrong input data: format of payload is incorrect");
                    }
                    if constexpr (std::is_same<ResponseT, void>::value) {
                        it->run(cmd);
                    } else {
                        it->run(cmd, response.out);
                    }
                } else if constexpr (!std::is_same<ResponseT, void>::value) {
                    it->run(response.out);
                }
            } else {
                throw Error("Not a correct task");
            }

            response.status = disco::Message::Status::Ok;
            if (auto res = m_bus->reply(Channel, m_in, response); !res) {
                logError(res.error());
            }
        } catch (const Error& err) {
            logError("Error: %s", err.what());
            response.setError(err.what());
            if (auto res = m_bus->reply(Channel, m_in, response); !res) {
                logError(res.error());
            }
        }
    }

protected:
    disco::Message     m_in;
    disco::MessageBus* m_bus = nullptr;
};

class AutoDiscovery;

template <typename T, typename InputT, typename ResponseT>
class AutoTask : public Task<T, InputT, ResponseT>
{
public:
    AutoTask(const disco::Message& in, disco::MessageBus& bus, AutoDiscovery& autoDiscovery)
          : m_autoDiscovery(&autoDiscovery)
    {
        this->m_in = in;
        this->m_bus = &bus;
    }

    AutoTask() = default;

protected:
    AutoDiscovery* m_autoDiscovery = nullptr;
};

} // namespace fty::disco::job
