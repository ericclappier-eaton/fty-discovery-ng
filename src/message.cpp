#include "message.h"
#include <fty_common_messagebus_message.h>

namespace fty {

// ===========================================================================================================

template <typename T>
struct identify
{
    using type = T;
};

template <typename K, typename V>
static V value(const std::map<K, V>& map, const typename identify<K>::type& key,
    const typename identify<V>::type& def = {})
{
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return def;
}

// ===========================================================================================================

Message::Message(const messagebus::Message& msg)
    : pack::Node::Node()
{
    meta.to            = value(msg.metaData(), messagebus::Message::TO);
    meta.from          = value(msg.metaData(), messagebus::Message::FROM);
    meta.replyTo       = value(msg.metaData(), messagebus::Message::REPLY_TO);
    meta.subject       = value(msg.metaData(), messagebus::Message::SUBJECT);
    meta.timeout       = value(msg.metaData(), messagebus::Message::TIMEOUT);
    meta.correlationId = value(msg.metaData(), messagebus::Message::CORRELATION_ID);

    meta.status.fromString(value(msg.metaData(), messagebus::Message::STATUS, "ok"));

    for (const auto& data : msg.userData()) {
        userData.append(data);
    }
}

messagebus::Message Message::toMessageBus() const
{
    messagebus::Message msg;

    for (const auto& data : userData) {
        msg.userData().emplace_back(data);
    }

    msg.metaData()[messagebus::Message::TO]             = meta.to;
    msg.metaData()[messagebus::Message::FROM]           = meta.from;
    msg.metaData()[messagebus::Message::REPLY_TO]       = meta.replyTo;
    msg.metaData()[messagebus::Message::SUBJECT]        = meta.subject;
    msg.metaData()[messagebus::Message::TIMEOUT]        = meta.timeout;
    msg.metaData()[messagebus::Message::CORRELATION_ID] = meta.correlationId;
    msg.metaData()[messagebus::Message::STATUS]         = meta.status.asString();

    return msg;
}

// ===========================================================================================================

} // namespace fty
