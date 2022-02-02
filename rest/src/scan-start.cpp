#include "scan-start.h"
#include "commands.h"
#include "discovery-rest.h"
#include "message-bus.h"
#include <fty/rest/component.h>

namespace fty::disco::start {

unsigned Scan::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    commands::scan::start::In in;
    if (auto ret = pack::json::deserialize(m_request.body(), in); !ret) {
        throw rest::errors::Internal(ret.error());
    }

    fty::disco::MessageBus bus;
    if (auto res = bus.init(AgentName); !res) {
        throw rest::errors::Internal(res.error());
    }

    fty::disco::Message msg = message(fty::disco::commands::scan::start::Subject);

    msg.setData(*pack::json::serialize(in));

    auto ret = bus.send(Channel, msg);
    if (!ret) {
        throw rest::errors::Internal(ret.error());
    }

    commands::scan::start::Out data;

    auto info = pack::json::deserialize(ret->userData.asString(), data);
    if (!info) {
        throw rest::errors::Internal(info.error());
    }
    if (data.status != commands::scan::stop::Out::Status::SUCCESS) {
        throw rest::errors::Internal(data.data.value());
    }

    return HTTP_ACCEPTED;
}

} // namespace fty::disco::start

registerHandler(fty::disco::start::Scan)
