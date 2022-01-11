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

#include "asset.h"
#include "message-bus.h"
#include <fty/rest/component.h>

namespace fty {

unsigned AssetRest::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    commands::assets::In param;
    if (auto res = pack::json::deserialize(m_request.body(), param); !res) {
        throw rest::errors::BadRequestDocument(res.error());
    }

    if (auto asset = assets(param)) {
        m_reply << *asset << "\n\n";
        return HTTP_OK;
    } else {
        throw rest::errors::Internal(asset.error());
    }
}

Expected<std::string> AssetRest::assets(const commands::assets::In& param)
{
    disco::MessageBus bus;
    if (auto res = bus.init("discovery_rest"); !res) {
        return unexpected(res.error());
    }

    disco::Message msg;
    msg.userData.setString(*pack::json::serialize(param));
    msg.meta.to      = "discovery-ng";
    msg.meta.subject = commands::assets::Subject;
    msg.meta.from    = "discovery_rest";

    if (Expected<disco::Message> resp = bus.send(fty::Channel, msg)) {
        if (resp->meta.status == disco::Message::Status::Error) {
            return unexpected(resp->userData.asString());
        }
        return resp->userData.asString();
    } else {
        return unexpected(resp.error());
    }
}

} // namespace fty

registerHandler(fty::AssetRest)
