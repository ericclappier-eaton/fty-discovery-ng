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

#pragma once
#include <fty/messagebus2/amqp/MessageBusAmqp.h>
#include <fty/messagebus2/Message.h>
#include "message.h"

#include <functional>
#include <memory>
#include <mutex>

// =====================================================================================================================

/*namespace messagebus2 {
class MessageBus2;
class Message;
} // namespace messagebus*/

// =====================================================================================================================

namespace fty::disco {
    
const std::string FTY_DISCOVERY_NG_AMQP {"fty-discover-ng"};    

/// Common message bus temporary wrapper
class MessageBusAmqpClient
{
public:
    //static constexpr const char* ENDPOINT = "ipc://@/malamute";

    MessageBusAmqpClient();
    ~MessageBusAmqpClient();

    [[nodiscard]] Expected<void> init(const std::string& clientName = FTY_DISCOVERY_NG_AMQP, const std::string& endpoint = "");
    void shutdown();

    [[nodiscard]] std::string getEndpoint() { return m_endpoint; };

    [[nodiscard]] Expected<Message> send(const std::string& queue, const Message& msg, int timeoutSec = 60);                
    [[nodiscard]] Expected<void>    reply(const std::string& queue, const Message& req, const Message& answ);
    ///[[nodiscard]] Expected<Message> recieve(const std::string& queue);

    /*template <typename Func, typename Cls>
    [[nodiscard]] Expected<void> subsribe(const std::string& queue, Func&& fnc, Cls* cls)
    {
        return subsribe(queue, [f = std::move(fnc), c = cls](const fty::messagebus2::Message& msg) -> void {
            //Message msg2;
            //msg2.fromMessageBus2(msg);
            //std::invoke(f, *c, msg2);
            std::invoke(f, *c, Message(msg));
        });
    }*/

private:
    Expected<void> subsribe(const std::string& queue, std::function<void(const fty::messagebus2::Message&)>&& func);

private:
    ///std::unique_ptr<messagebus::MessageBus> m_bus;
    std::mutex                              m_mutex;
    ///std::string                             m_actorName;
    ///std::string                             m_endpoint;
    
    std::string m_clientName;
    std::string m_endpoint;
    //ClientData* m_clientData {nullptr}; // owned data
    std::unique_ptr<fty::messagebus2::amqp::MessageBusAmqp> m_bus; // msgbus instance
};

} // namespace fty
