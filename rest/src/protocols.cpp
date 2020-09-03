#include "protocols.h"
#include "commands.h"
#include "message-bus.h"
#include <tnt/http.h>

namespace fty {

unsigned Protocols::run()
{
    //    auto user = global<UserInfo>("UserInfo user");
    //    if (auto ret = checkPermissions(**user, m_permissions); !ret) {
    //        m_reply.out() << ret.error().message << "\n";
    //        return ret.error().code;
    //    }

    auto address = queryValue<std::string, true>("address");
    if (!address) {
        m_reply.out() << address.error().message << "\n\n";
        return address.error().code;
    }

    if (auto list = protocols(*address)) {
        m_reply.setContentType("application/json;charset=UTF-8");
        m_reply.out() << *pack::json::serialize(*list) << "\n\n";
        return HTTP_OK;
    } else {
        m_reply.out() << list.error() << "\n\n";
        return HTTP_INTERNAL_SERVER_ERROR;
    }
}

Expected<pack::StringList> Protocols::protocols(const std::string& address)
{
    fty::MessageBus bus;
    if (auto res = bus.init("discovery_rest"); !res) {
        return unexpected(res.error());
    }

    commands::protocols::In in;
    in.address = address;

    fty::Message msg;
    msg.userData.setString(*pack::json::serialize(in));

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
