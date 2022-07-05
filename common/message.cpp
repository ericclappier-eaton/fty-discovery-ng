/*  =========================================================================
    message.cpp - Common message bus wrapper

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

#include "message.h"
#include <fty_common_messagebus_message.h>
#include <fty/messagebus/Message.h>

namespace fty::disco {

// ===========================================================================================================

template <typename T>
struct identify
{
    using type = T;
};

template <typename K, typename V>
static V value(
    const std::map<K, V>& map, const typename identify<K>::type& key, const typename identify<V>::type& def = {})
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

    if (!msg.userData().empty()) {
        userData.setString(msg.userData().front());
    }
}

messagebus::Message Message::toMessageBus() const
{
    messagebus::Message msg;

    msg.userData().emplace_back(userData.asString());

    msg.metaData()[messagebus::Message::TO]             = meta.to;
    msg.metaData()[messagebus::Message::FROM]           = meta.from;
    msg.metaData()[messagebus::Message::REPLY_TO]       = meta.replyTo;
    msg.metaData()[messagebus::Message::SUBJECT]        = meta.subject;
    msg.metaData()[messagebus::Message::TIMEOUT]        = meta.timeout;
    msg.metaData()[messagebus::Message::CORRELATION_ID] = meta.correlationId;
    msg.metaData()[messagebus::Message::STATUS]         = meta.status.asString();

    return msg;
}

Message::Message(const fty::messagebus2::Message& msg)
    : pack::Node::Node()
{
    meta.to            = value(msg.metaData(), fty::messagebus2::TO);
    meta.from          = value(msg.metaData(), fty::messagebus2::FROM);
    meta.replyTo       = value(msg.metaData(), fty::messagebus2::REPLY_TO);
    meta.subject       = value(msg.metaData(), fty::messagebus2::SUBJECT);
    meta.timeout       = value(msg.metaData(), fty::messagebus2::TIME_OUT);
    meta.correlationId = value(msg.metaData(), fty::messagebus2::CORRELATION_ID);

    meta.status.fromString(value(msg.metaData(), fty::messagebus2::STATUS, "ok"));

    if (!msg.userData().empty()) {
        userData.setString(msg.userData());
    }
}
/*
void Message::fromMessageBus2(const fty::messagebus2::Message& msg)
{
    meta.to            = value(msg.metaData(), fty::messagebus2::TO);
    meta.from          = value(msg.metaData(), fty::messagebus2::FROM);
    meta.replyTo       = value(msg.metaData(), fty::messagebus2::REPLY_TO);
    meta.subject       = value(msg.metaData(), fty::messagebus2::SUBJECT);
    meta.timeout       = value(msg.metaData(), fty::messagebus2::TIME_OUT);
    meta.correlationId = value(msg.metaData(), fty::messagebus2::CORRELATION_ID);

    meta.status.fromString(value(msg.metaData(), fty::messagebus2::STATUS, "ok"));

    if (!msg.userData().empty()) {
        userData.setString(msg.userData());
    }
}*/

fty::messagebus2::Message Message::toMessageBus2() const
{
    fty::messagebus2::Message msg;

    msg.userData(userData.asString());

    msg.metaData()[fty::messagebus2::TO]             = meta.to;
    msg.metaData()[fty::messagebus2::FROM]           = meta.from;
    msg.metaData()[fty::messagebus2::REPLY_TO]       = meta.replyTo;
    msg.metaData()[fty::messagebus2::SUBJECT]        = meta.subject;
    msg.metaData()[fty::messagebus2::TIME_OUT]       = meta.timeout;
    msg.metaData()[fty::messagebus2::CORRELATION_ID] = meta.correlationId;
    msg.metaData()[fty::messagebus2::STATUS]         = meta.status.asString();

    return msg;
}

void Message::setData(const std::string& data)
{
    userData.clear();
    userData.setString(data);
}

// ===========================================================================================================

} // namespace fty
