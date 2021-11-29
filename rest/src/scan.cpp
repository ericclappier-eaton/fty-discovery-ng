#include "scan.h"
#include "commands.h"
#include "discovery-config.h"
#include "message-bus.h"
#include "discovery-rest.h"
#include <fty/rest/component.h>

namespace fty::disco {

unsigned Scan::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    commands::scan::In in;
    if (m_request.type() == fty::rest::Request::Type::Post) {
        if (auto ret = pack::json::deserialize(m_request.body(), in); !ret) {
            throw rest::errors::Internal(ret.error());
        }
    }

    fty::disco::MessageBus bus;
    if (auto res = bus.init(AgentName); !res) {
        throw rest::errors::Internal(res.error());
    }

    fty::disco::Message msg = message(fty::disco::commands::scan::Subject);

    msg.setData(*pack::json::serialize(in));

    auto ret = bus.send(fty::Channel, msg);
    if (!ret) {
        throw rest::errors::Internal(ret.error());
    }

    commands::scan::Out data;
    auto                info = pack::json::deserialize(ret->userData.asString(), data);
    if (!info) {
        throw rest::errors::Internal(info.error());
    }

    m_reply << *pack::json::serialize(data);

    return HTTP_OK;
}

} // namespace fty::disco

registerHandler(fty::disco::Scan)
