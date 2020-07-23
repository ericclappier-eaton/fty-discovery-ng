/*  =========================================================================
    message-bus.h - Common message bus wrapper

    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
 */

#include "message-bus.h"
#include "message.h"
#include <fty/fty-log.h>
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
    answ.meta.to            = req.meta.from;
    answ.meta.from          = req.meta.to;

    m_bus->sendReply(queue, answ.toMessageBus());
}

Message MessageBus::recieve(const std::string& queue)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Message                     ret;
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

} // namespace fty
