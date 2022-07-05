/*  ====================================================================================================================
    message.h - Common message bus wrapper

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
    ====================================================================================================================
*/

#pragma once
#include <pack/pack.h>

//#include <fty/messagebus/Message.h>

// =====================================================================================================================

namespace messagebus {
class Message;
}

namespace fty::messagebus2 {
class Message;
}

// =====================================================================================================================

namespace fty::disco {

/// Common message bus message temporary wrapper
class Message : public pack::Node
{
public:
    enum class Status
    {
        Ok,
        Error
    };

    struct Meta : public pack::Node
    {
        pack::String         replyTo       = FIELD("reply-to");
        mutable pack::String from          = FIELD("from");
        mutable pack::String to            = FIELD("to");
        pack::String         subject       = FIELD("subject");
        pack::Enum<Status>   status        = FIELD("status");
        pack::String         timeout       = FIELD("timeout");
        mutable pack::String correlationId = FIELD("correlation-id");

        using pack::Node::Node;
        META(Meta, replyTo, from, to, subject, status, timeout, correlationId);
    };

public:
    pack::Binary userData = FIELD("user-data");
    Meta         meta     = FIELD("meta-data");

public:
    using pack::Node::Node;
    META(Message, userData, meta);

public:
    explicit Message(const messagebus::Message& msg);
    messagebus::Message toMessageBus() const;
    
    explicit Message(const fty::messagebus2::Message& msg);
    //void fromMessageBus2(const fty::messagebus2::Message& msg);
    fty::messagebus2::Message toMessageBus2() const;
    
    void                setData(const std::string& data);
};

inline std::ostream& operator<<(std::ostream& ss, Message::Status status)
{
    switch (status) {
    case Message::Status::Ok:
        ss << "ok";
        break;
    case Message::Status::Error:
        ss << "ko";
        break;
    }
    return ss;
}

inline std::istream& operator>>(std::istream& ss, Message::Status& status)
{
    std::string str;
    ss >> str;
    if (str == "ok") {
        status = Message::Status::Ok;
    } else if (str == "ko") {
        status = Message::Status::Error;
    }
    return ss;
}

} // namespace fty

// =====================================================================================================================
