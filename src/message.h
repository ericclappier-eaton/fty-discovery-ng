#pragma once
#include <pack/pack.h>

namespace messagebus {
class Message;
}

namespace fty {

class Message : public pack::Node
{
public:
    using pack::Node::Node;

    enum class Status
    {
        ok,
        ko
    };

    explicit Message(const messagebus::Message& msg);

    struct Meta : public Node
    {
        using pack::Node::Node;

        pack::String         replyTo       = FIELD("reply-to");
        pack::String         from          = FIELD("from");
        pack::String         to            = FIELD("to");
        pack::String         subject       = FIELD("subject");
        pack::Enum<Status>   status        = FIELD("status");
        pack::String         timeout       = FIELD("timeout");
        mutable pack::String correlationId = FIELD("correlation-id");

        META_FIELDS(Meta, replyTo, from, to, subject, status, timeout, correlationId)
    };

    pack::StringList userData = FIELD("user-data");
    Meta             meta     = FIELD("meta-data");

    META_FIELDS(Message, userData, meta)

    messagebus::Message toMessageBus() const;
};

} // namespace fty
