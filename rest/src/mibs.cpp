#include "mibs.h"
#include "commands.h"
#include "message-bus.h"
#include <fty_common_rest_utils_web.h>

namespace fty {

unsigned Mibs::run()
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

    auto port = queryValue<uint32_t>("port");
    auto secw = queryValue<std::string>("secw_credential_id");

    if (auto list = mibs(*address, port ? *port : 161, secw ? *secw : "")) {
        m_reply.out() << *pack::json::serialize(*list) << "\n\n";
        return HTTP_OK;
    } else {
        m_reply.out() << list.error() << "\n\n";
        return HTTP_INTERNAL_SERVER_ERROR;
    }
}

Expected<pack::StringList> Mibs::mibs(const std::string& address, uint32_t port, const std::string& secw)
{
    fty::MessageBus bus;
    if (auto res = bus.init("discovery_rest"); !res) {
        return unexpected(res.error());
    }

    commands::mibs::In in;
    in.address      = address;
    in.port         = port;
    in.credentialId = secw;

    fty::Message msg;
    msg.userData.setString(*pack::json::serialize(in));
    msg.meta.to      = "discovery-ng";
    msg.meta.subject = commands::mibs::Subject;
    msg.meta.from    = "discovery_rest";

    if (Expected<fty::Message> resp = bus.send(fty::Channel, msg)) {
        if (resp->meta.status == fty::Message::Status::Error) {
            return unexpected(resp->userData.asString());
        }

        if (auto res = resp->userData.decode<commands::mibs::Out>()) {
            return res.value();
        } else {
            return unexpected(res.error());
        }
    } else {
        return unexpected(resp.error());
    }
}

} // namespace fty

registerHandler(fty::Mibs)
