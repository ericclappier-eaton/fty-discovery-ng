#pragma once
#include "message.h"

namespace fty::disco {

inline fty::disco::Message message(const std::string& subject, const std::string& clientName)
{
    fty::disco::Message msg;

    msg.meta.replyTo = clientName;
    msg.meta.from    = clientName;
    msg.meta.subject = subject;
    msg.meta.to      = "fty-discovery-ng";

    return msg;
}

}
