#pragma once
#include "message.h"

namespace fty::disco {

static constexpr const char* AgentName = "discovery-ng_rest";


inline fty::disco::Message message(const std::string& subj)
{
    fty::disco::Message msg;
    msg.meta.to      = "fty-discovery-ng";
    msg.meta.replyTo = AgentName;
    msg.meta.subject = subj;
    msg.meta.from    = AgentName;
    return msg;
}

}
