/*  =========================================================================
    Copyright (C) 2021 Eaton.

    This software is confidential and licensed under Eaton Proprietary License
    (EPL or EULA).
    This software is not authorized to be used, duplicated or disclosed to
    anyone without the prior written permission of Eaton.
    Limitations, restrictions and exclusions of the Eaton applicable standard
    terms and conditions, such as its EPL and EULA, apply.
    =========================================================================
*/
#include "create-asset.h"
#include <fty_log.h>

namespace fty::asset::create {
fty::Expected<Response> run(fty::disco::MessageBus& bus, const std::string& from, const Request& req)
{
    fty::disco::Message msg;
    msg.meta.subject = Subject;
    msg.meta.to      = agent::Name;
    msg.meta.replyTo = from;

    if (auto json = pack::json::serialize(req, pack::Option::WithDefaults); !json) {
        return fty::unexpected(json.error());
    } else {
        logInfo("Create asset:  json={}: ", *json);
        msg.userData.setString(*json);
    }

    // query ID of all assets created
    Response resp;
    if (auto reply = bus.send(Queue, msg); !reply) {
        return fty::unexpected(reply.error());
    } else {
        const auto& data = reply.value().userData;
        if (data.size() < 1) {
            return fty::unexpected("Response has no data");
        } else {
            logInfo("Create asset:  data.asString()={}: ", data.asString());
            try {
            if (auto ret = pack::json::deserialize(data.asString(), resp); !ret) {
                logInfo("Create asset: NOK");
                return fty::unexpected(ret.error());
            }
            }
            catch (std::exception& ex) {
                return fty::unexpected("Error {}", ex.what());
            }
        }
    }
    logInfo("Create asset: OK");
    return resp;
}
} // namespace fty::asset::create
