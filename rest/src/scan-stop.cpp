#include "scan-stop.h"
#include "commands.h"
#include "discovery-rest.h"
#include "message-bus.h"
#include <fty/rest/component.h>

namespace fty::disco::stop {

unsigned Scan::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    fty::disco::MessageBus bus;
    if (auto res = bus.init(AgentName); !res) {
        throw rest::errors::Internal(res.error());
    }

    fty::disco::Message msg = message(commands::scan::stop::Subject);

    auto ret = bus.send(Channel, msg);
    if (!ret) {
        throw rest::errors::Internal(ret.error());
    }

    commands::scan::stop::Out data;

    auto info = pack::json::deserialize(ret->userData.asString(), data);
    if (!info) {
        throw rest::errors::Internal(info.error());
    }
    if (data.status != commands::scan::stop::Out::Status::SUCCESS) {
        throw rest::errors::Internal(data.data.value());
    }

    return HTTP_OK;
}

} // namespace fty::disco::stop

registerHandler(fty::disco::stop::Scan)
