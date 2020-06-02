#pragma once
#include "message.h"
#include <memory>
#include <functional>
#include <mutex>

namespace messagebus {
class MessageBus;
class Message;
}

namespace fty {

class MessageBus
{
public:
    static constexpr const char* endpoint = "ipc://@/malamute";
public:
    MessageBus();
    ~MessageBus();

    void init(const std::string& actorName);

    Message send(const std::string& queue, const Message& msg);
    void reply(const std::string& queue, const Message& req, const Message& answ);
    Message recieve(const std::string& queue);

    template<typename Func, typename Cls>
    void subsribe(const std::string& queue, Func&& fnc, Cls* cls)
    {
        subsribe(queue, [f = std::move(fnc), c = cls](const messagebus::Message& msg) -> void {
            std::invoke(f, *c, Message(msg));
        });
    }

private:
    void subsribe(const std::string& queue, std::function<void(const messagebus::Message&)>&& func);

private:
    std::unique_ptr<messagebus::MessageBus> m_bus;
    std::mutex m_mutex;
};

} // namespace fty
