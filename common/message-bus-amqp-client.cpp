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

#include "message-bus-amqp-client.h"
#include "message.h"
#include <fty_log.h>
///#include <fty_common_messagebus_exception.h>
///#include <fty_common_messagebus_interface.h>
///#include <fty_common_messagebus_message.h>

namespace fty::disco {
       
using namespace fty::messagebus2::amqp;    

MessageBusAmqpClient::MessageBusAmqpClient() = default;

#if 0
Expected<void> MessageBusAmqpClient::init(const std::string& actorName, const std::string& endpoint)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        m_bus = std::unique_ptr<messagebus::MessageBus>(messagebus::MlmMessageBus(endpoint, actorName));
        m_bus->connect();
        m_actorName = actorName;
        m_endpoint = endpoint;
        return {};
    } catch (std::exception& ex) {
        return unexpected(ex.what());
    }
}
#endif

void MessageBusAmqpClient::shutdown()
{
    m_bus = nullptr;
}

MessageBusAmqpClient::~MessageBusAmqpClient()
{
}

#if 0
Expected<Message> MessageBusAmqpClient::send(const std::string& queue, const Message& msg, int timeoutSec)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (msg.meta.correlationId.empty()) {
        msg.meta.correlationId = messagebus::generateUuid();
    }
    msg.meta.from = m_actorName;
    try {
        Message m(m_bus->request(queue, msg.toMessageBus(), timeoutSec));
        if (m.meta.status == Message::Status::Error) {
            return unexpected(*m.userData.decode<std::string>());
        }
        return Expected<Message>(m);
    } catch (messagebus::MessageBusException& ex) {
        return unexpected(ex.what());
    }
}
#endif

#if 0
Expected<void> MessageBusAmqpClient::reply(const std::string& queue, const Message& req, const Message& answ)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    answ.meta.correlationId = req.meta.correlationId;
    answ.meta.to            = req.meta.from;
    answ.meta.from          = req.meta.to;

    try {
        m_bus->sendReply(queue, answ.toMessageBus());
        return {};
    } catch (messagebus::MessageBusException& ex) {
        return unexpected(ex.what());
    }
}
#endif

#if 0
// NOT IMPLEMENTED
Expected<Message> MessageBusAmqpClient::recieve(const std::string& queue)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Message                     ret;
    try {
        m_bus->receive(queue, [&ret](const messagebus::Message& msg) {
            ret = Message(msg);
        });
        return Expected<Message>(ret);
    } catch (messagebus::MessageBusException& ex) {
        return unexpected(ex.what());
    }
}
#endif

#if 0
Expected<void> MessageBusAmqpClient::subsribe(const std::string& queue, std::function<void(const messagebus::Message&)>&& func)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        m_bus->subscribe(queue, func);
        return {};
    } catch (messagebus::MessageBusException& ex) {
        return unexpected(ex.what());
    }
}
#endif



////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// request listener callback
void MessageBusAmqpClient::onRequest(const fty::messagebus::Message& request)
{
    using namespace etn::licensing::message;

    if (!(m_clientData && m_clientData->common && m_clientData->licensing)) {
        logError("onRequest: bad client data");
        return;
    }

    const std::lock_guard<std::mutex> lock(m_clientData->common->protection_mutex);
    logDebug("request input message is:\n{}", request.toString());

    // get the json payload from request
    std::string jsonRequest{request.userData()};
    //logDebug("jsonRequest: '{}'", jsonRequest);

    Base *reply_pack = nullptr;
    std::string jsonReply;

    // read request
    try {
        etn::licensing::command::In in;
        if (auto ret = pack::json::deserialize(jsonRequest, in); !ret) {
            throw std::runtime_error(ret.error());
        }
        //auto subject = request.subject();

        bool republish = false;
        // get message type
        std::stringstream ss;
        ss << in.messageType;
        auto messageType = ss.str();

        // construct message to send
        constructMessage(
            m_clientData->common,
            m_clientData->licensing,
            messageType.c_str(),
            in.command.value().c_str(),
            in.option.value().c_str(),
            &reply_pack,
            republish);

        // if needed, republish limitations on stream
        if (republish) {
            if (auto ret2 = publishLimitations(); !ret2) {
                logError(ret2.error());
            }
        }

        // test reply before serialize
        if (!reply_pack) {
            throw std::runtime_error("reply is NULL");
        }

        // serialize response
        if (auto res = pack::json::serialize(*reply_pack, pack::Option::WithDefaults); !res) {
            throw std::runtime_error(res.error());
        }
        else {
            jsonReply = *res;
        }
    }
    catch (const std::exception& ex) {
        logError("read request failed (ex: '{}')", ex.what());
    }

    // send response
    try {
        if (!m_instance) {
            throw std::runtime_error("instance is NULL");
        }

        // WA: messagebus2::amqp: SUBJECT metadata is required
        auto req {request}; // copy
        if (req.subject().empty()) {
            logInfo("req.subject().empty()");
            req.subject("Licensing_default_subject");
            // TODO needed ???
            //std::string JASubject{req.getMetaDataValue("JMS_AMQP_Subject")};
            //req.subject(JASubject.empty() ? "Licensing_default_subject" : JASubject);
        }
        // Set response status to "ok"
        req.status("ok");

        auto response = req.buildReply(jsonReply);
        if (!response) {
            throw std::runtime_error("buildReply() failed (" + response.error() + ")");
        }

        // TODO Needed ???: WA resp. 'x_status' metadata shall be set to 'ok'
        /*if (response.value().getMetaDataValue("x_status").empty()) {
            logDebug("================ response.x_status().empty()");
            response.value().setMetaDataValue("x_status", "ok");
        }*/

        logDebug("built response message is:\n{}", response.value().toString());

        auto res = m_instance->send(response.value());
        if (!res) {
            throw std::runtime_error(std::string("Error during send message: ") + fty::messagebus::to_string(res.error()));
        }

        logInfo("send() succeeded");
    }
    catch (const std::exception& ex) {
        logError("send response failed (ex: '{}')", ex.what());
    }

    if (reply_pack) delete reply_pack;
}
#endif

// send message

fty::Expected<Message> MessageBusAmqpClient::send(const std::string& queue, const Message& msg, int timeoutSec)
{
    if (!m_bus) {
        return fty::unexpected("instance is NULL");
    }

    try { 
        // TODO: queue ?
        //fty::messagebus::Message message;
        // send the message        
        auto sendRet = m_bus->request(msg.toMessageBus2(), timeoutSec);
        if (!sendRet) {
            throw std::runtime_error("Error while sending " + fty::messagebus2::to_string(sendRet.error()));
        }
        //Message msg2;
        //msg2.fromMessageBus2(*sendRet);
        //return msg2;
        return Message(*sendRet); 
    }
    catch (const std::exception& ex) {
        return fty::unexpected("Error detected during publish (ex: '{}')", ex.what());
    }

    //return {};
}

Expected<void> MessageBusAmqpClient::reply(const std::string& queue, const Message& req, const Message& answ)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    answ.meta.correlationId = req.meta.correlationId;
    answ.meta.to            = req.meta.from;
    answ.meta.from          = req.meta.to;

    try {
        //TODO: queue ???
        auto res = m_bus->send(/*queue,*/ answ.toMessageBus2());
        if (!res) {
            return fty::unexpected("Error detected during sens ({})", res.error());
        }
        return {};
    } catch (const std::exception& ex) {
        return unexpected(ex.what());
    }
}

// subscribe to message
Expected<void> MessageBusAmqpClient::subsribe(const std::string& queue, std::function<void(const fty::messagebus2::Message&)>&& func)
{                                                                       
    std::lock_guard<std::mutex> lock(m_mutex);
    try {                
        // register callback (request listener on queue)
        //auto ret = m_bus->receive(queue, std::bind(func, std::ref(*this), std::placeholders::_1));
        std::function<void(const fty::messagebus2::Message&)> func2 = func;
        std::string rr;
        const std::string essai = std::const_cast<const std::string>(rr);
        auto ret = /*m_bus->connect();*/ m_bus->receive(queue, func2, std::const_cast<const std::string>(rr));
        if (!ret) {
            throw std::runtime_error(std::string("MessageBusAmqp() receive failed (") +
                fty::messagebus2::to_string(ret.error()) + std::string(")"));
        }
        logInfo("init register listener (queue: '{}', clientName: '{}')", queue, m_clientName);                                
        return {};
    } catch (const std::exception& ex) {
        return unexpected(ex.what());
    }
}

// initialization to receive request messages
fty::Expected<void> MessageBusAmqpClient::init(const std::string& clientName, const std::string& endpoint)
{
    if (m_bus) {
       return fty::unexpected("Error instance still initialised");
    }
    m_clientName = clientName;
    m_endpoint   = endpoint;

    try {
        // instantiate client
        m_bus = std::unique_ptr<MessageBusAmqp>(new MessageBusAmqp(m_clientName, m_endpoint));
        if (!m_bus) {
            throw std::runtime_error("MessageBusAmqp() creation failed");
        }
        logInfo("init creation successfully (clientName: '{}')", m_clientName);

        auto ret1 = m_bus->connect();
        if (!ret1) {
            throw std::runtime_error(std::string("MessageBusAmqp() connect failed (") +
                fty::messagebus2::to_string(ret1.error()) + std::string(")"));
        }
        logInfo("init creation successfully (clientName: '{}')", m_clientName);

#if 0
        // build queue address
        std::string queue{REQUEST_QUEUE_FORMAT + m_clientName};

        // register onRequest() callback (request listener on queue)
        auto ret2 = m_bus->receive(queue, std::bind(&MessageBusAmqpClient::onRequest, std::ref(*this), std::placeholders::_1));
        if (!ret2) {
            throw std::runtime_error(std::string("MessageBusAmqp() receive failed (") +
                fty::messagebus::to_string(ret2.error()) + std::string(")"));
        }
        logInfo("init register listener (queue: '{}', clientName: '{}')", queue, m_clientName);

//#if 0        
        // build topic address (token substitution)
        std::string topic{PUBLISH_TOPIC_FORMAT};
        {
            auto pos = topic.find(QTOKEN_CLIENTNAME);
            if (pos != std::string::npos) {
                topic.replace(pos, QTOKEN_CLIENTNAME.length(), m_clientName);
            }
            else {
                logWarn("clientName token ({}) not found in topic fmt ({})", QTOKEN_CLIENTNAME, PUBLISH_TOPIC_FORMAT);
            }
        }

        // register onPublish() callback (listener on topic)
        auto ret3 = m_instance->receive(topic, std::bind(&MessageBusAmqpClient::onPublish, std::ref(*this), std::placeholders::_1));
        if (!ret3) {
            throw std::runtime_error(std::string("MessageBusAmqp() receive failed (") +
                fty::messagebus::to_string(ret3.error()) + std::string(")"));
        }
        logInfo("init register listener (topic: '{}', clientName: '{}')", topic, m_clientName);

        // Execute task in charge of connection check
        startThreadCheckConnection();

        // Execute task in charge of republish limitations
        startThreadRepublish();
#endif        
    }
    catch (const std::exception& ex) {
        return fty::unexpected("init failed (clientName: '{}', ex: '{}')", m_clientName, ex.what());
    }

    logDebug("Init successfully (clientName: '{}')", m_clientName);
    return {};
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////










} // namespace fty
