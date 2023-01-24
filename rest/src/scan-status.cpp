#include "scan-status.h"
#include "commands.h"
#include "discovery-rest.h"
#include "message-bus.h"
#include "discovery-rest.h"
#include <fty/rest/component.h>
#include <sys/types.h> //gettid()

namespace fty::disco::status {

unsigned Scan::run()
{
    static const std::string ACTOR_NAME = "fty-discovery-ng-rest_scan-status";
    std::string clientNameWithThreadId(ACTOR_NAME + "-" + std::to_string(gettid()));

    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    fty::disco::MessageBus bus;
    if (auto res = bus.init(clientNameWithThreadId); !res) {
        throw rest::errors::Internal(res.error());
    }

    fty::disco::Message msg = message(commands::scan::status::Subject, clientNameWithThreadId);

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
