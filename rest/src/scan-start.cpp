#include "scan-start.h"
#include "commands.h"
#include "discovery-rest.h"
#include "message-bus.h"
#include "discovery-rest.h"
#include <fty/rest/component.h>
#include <sys/types.h> //gettid()

namespace fty::disco::start {

unsigned Scan::run()
{
    static const std::string ACTOR_NAME = "fty-discovery-ng-rest_scan-start";
    std::string clientNameWithThreadId(ACTOR_NAME + "-" + std::to_string(gettid()));

    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    if (m_request.body().empty()) {
        throw rest::errors::Internal("Bad request");
    }

    fty::disco::MessageBus bus;
    if (auto res = bus.init(clientNameWithThreadId); !res) {
        throw rest::errors::Internal(res.error());
    }

    fty::disco::Message msg = message(fty::disco::commands::scan::start::Subject, clientNameWithThreadId);
    msg.setData(m_request.body());
    auto ret = bus.send(Channel, msg);
    if (!ret) {
        throw rest::errors::Internal(ret.error());
    }

    commands::scan::start::Out data;

    auto info = pack::json::deserialize(ret->userData.asString(), data);
    if (!info) {
        throw rest::errors::Internal(info.error());
    }
    if (data.status != commands::scan::stop::Out::Status::Success) {
        throw rest::errors::Internal(data.data.value());
    }

    return HTTP_ACCEPTED;
}

} // namespace fty::disco::start

registerHandler(fty::disco::start::Scan)
