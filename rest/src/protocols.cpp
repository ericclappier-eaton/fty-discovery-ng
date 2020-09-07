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

#include "protocols.h"
#include "commands.h"
#include "message-bus.h"
#include <tnt/http.h>

namespace fty {

unsigned Protocols::run()
{
    auto user = global<UserInfo>("UserInfo user");
    if (auto ret = checkPermissions(**user, m_permissions); !ret) {
        m_reply.out() << ret.error().message << "\n";
        return ret.error().code;
    }

    commands::protocols::In param;
    if (auto res = pack::json::deserialize(m_request.getBody(), param); !res) {
        auto err = rest::error("bad-input", m_request.getArg(0));
        m_reply.out() << err.message << "\n\n";
        return err.code;
    }

    if (auto list = protocols(param)) {
        m_reply.setContentType("application/json;charset=UTF-8");
        m_reply.out() << *pack::json::serialize(*list) << "\n\n";
        return HTTP_OK;
    } else {
        m_reply.out() << list.error() << "\n\n";
        return HTTP_INTERNAL_SERVER_ERROR;
    }
}

Expected<pack::StringList> Protocols::protocols(const commands::protocols::In& param)
{
    fty::MessageBus bus;
    if (auto res = bus.init("discovery_rest"); !res) {
        return unexpected(res.error());
    }

    fty::Message msg;
    msg.userData.setString(*pack::json::serialize(param));

    msg.meta.to      = "discovery-ng";
    msg.meta.subject = commands::protocols::Subject;
    msg.meta.from    = "discovery_rest";

    if (Expected<fty::Message> resp = bus.send(fty::Channel, msg)) {
        if (resp->meta.status == fty::Message::Status::Error) {
            return unexpected(resp->userData.asString());
        }

        if (auto res = resp->userData.decode<commands::protocols::Out>()) {
            return res.value();
        } else {
            return unexpected(res.error());
        }
    } else {
        return unexpected(resp.error());
    }
}

} // namespace fty

registerHandler(fty::Protocols)
