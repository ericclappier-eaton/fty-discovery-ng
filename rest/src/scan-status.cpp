#include "scan-status.h"
#include "commands.h"
#include "discovery-rest.h"
#include "message-bus.h"
#include <fty/rest/component.h>

namespace fty::disco::status {

unsigned Scan::run()
{
    static constexpr const char* ACTOR_NAME = "fty-discovery-ng-rest_scan-status";

    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    fty::disco::MessageBus bus;
    if (auto res = bus.init(ACTOR_NAME); !res) {
        throw rest::errors::Internal(res.error());
    }

    fty::disco::Message msg = message(commands::scan::status::Subject, ACTOR_NAME);

    auto ret = bus.send(Channel, msg);
    if (!ret) {
        throw rest::errors::Internal(ret.error());
    }

    commands::scan::status::Out data;
    auto info = pack::json::deserialize(ret->userData.asString(), data);
    if (!info) {
        throw rest::errors::Internal(info.error());
    }

    m_reply << *pack::json::serialize(data, pack::Option::WithDefaults);

    return HTTP_OK;
}

} // namespace fty::disco::status

registerHandler(fty::disco::status::Scan)
