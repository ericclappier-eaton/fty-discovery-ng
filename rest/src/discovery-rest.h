#pragma once
#include "common/message.h"

namespace fty::disco {

static constexpr const char* AgentName = "discovery-ng_rest";


inline fty::groups::Message message(const std::string& subj)
{
    fty::groups::Message msg;
    msg.meta.to      = "discovery-ng";
    msg.meta.replyTo = AgentName;
    msg.meta.subject = subj;
    msg.meta.from    = AgentName;
    return msg;
}

}