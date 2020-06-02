#include "message-bus.h"
#include "message.h"
#include <fty_common_messagebus_interface.h>
#include <fty_common_messagebus_message.h>

namespace fty {

MessageBus::MessageBus() = default;

void MessageBus::init(const std::string& actorName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bus = std::unique_ptr<messagebus::MessageBus>(messagebus::MlmMessageBus(endpoint, actorName));
    m_bus->connect();
}

MessageBus::~MessageBus()
{
}

Message MessageBus::send(const std::string& queue, const Message& msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (msg.meta.correlationId.empty()) {
        msg.meta.correlationId = messagebus::generateUuid();
    }
    return Message(m_bus->request(queue, msg.toMessageBus(), 1000));
}

void MessageBus::reply(const std::string& queue, const Message& req, const Message& answ)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    answ.meta.correlationId = req.meta.correlationId;
    m_bus->sendReply(queue, answ.toMessageBus());
}

Message MessageBus::recieve(const std::string& queue)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Message ret;
    m_bus->receive(queue, [&ret](const messagebus::Message& msg) {
        ret = Message(msg);
    });
    return ret;
}

void MessageBus::subsribe(const std::string& queue, std::function<void(const messagebus::Message&)>&& func)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bus->subscribe(queue, func);
}


}
