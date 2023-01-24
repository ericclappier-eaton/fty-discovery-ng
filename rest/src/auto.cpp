/*  ====================================================================================================================
    auto.cpp -

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

#include "auto.h"
#include "message-bus.h"
#include "discovery-rest.h"
#include <fty/rest/component.h>
#include <sys/types.h> //gettid()

namespace fty::disco {

unsigned DiscoveryAutoRest::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    commands::discoveryauto::In param;
    if (auto res = pack::json::deserialize(m_request.body(), param); !res) {
        throw rest::errors::BadRequestDocument(res.error());
    }

    if (auto discoAuto = discoveryAuto(param)) {
        m_reply << *discoAuto << "\n\n";
        return HTTP_OK;
    } else {
        throw rest::errors::Internal(discoAuto.error());
    }
}

Expected<std::string> DiscoveryAutoRest::discoveryAuto(const commands::discoveryauto::In& param)
{
    static const std::string ACTOR_NAME = "fty-discovery-ng-rest_auto";
    std::string clientNameWithThreadId(ACTOR_NAME + "-" + std::to_string(gettid()));

    disco::MessageBus bus;
    if (auto res = bus.init(clientNameWithThreadId); !res) {
        return unexpected(res.error());
    }

    disco::Message msg;
    msg.userData.setString(*pack::json::serialize(param));

    msg.meta.to      = AGENT_DISCOVERY_NAME;
    msg.meta.subject = commands::discoveryauto::Subject;

    if (Expected<disco::Message> resp = bus.send(Channel, msg)) {
        if (resp->meta.status == disco::Message::Status::Error) {
            return unexpected(resp->userData.asString());
        }
        return resp->userData.asString();
    } else {
        return unexpected(resp.error());
    }
}

} // namespace fty::disco

registerHandler(fty::disco::DiscoveryAutoRest)
