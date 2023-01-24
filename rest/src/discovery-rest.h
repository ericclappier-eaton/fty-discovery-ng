#pragma once
#include "message.h"

namespace fty::disco {

static const char* AGENT_DISCOVERY_NAME = "fty-discovery-ng";

inline fty::disco::Message message(const std::string& subject, const std::string& clientName)
{
    fty::disco::Message msg;

    msg.meta.replyTo = clientName;
    msg.meta.from    = clientName;
    msg.meta.subject = subject;
    msg.meta.to      = AGENT_DISCOVERY_NAME;

    return msg;
}

}
