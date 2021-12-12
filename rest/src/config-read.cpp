#include "config-read.h"
#include "commands.h"
#include "discovery-config-manager.h"
#include <fty/rest/component.h>

namespace fty::disco {

unsigned ConfigRead::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    } 

    commands::config::read::In in;

    if (auto key = m_request.queryArg<std::string>("key"); !key) {
        if (auto body = m_request.body(); body.empty()) {
            throw rest::errors::RequestParamRequired("key");
        } else if (auto ret = pack::json::deserialize(body, in); !ret) {
            throw rest::errors::Internal(ret.error());
        }
    } else {
        in.append(*key);
    }

    commands::config::read::Out out;
    ConfigDiscoveryManager::instance().load();

    if (auto ret = ConfigDiscoveryManager::instance().commandRead(in); !ret) {
        throw rest::errors::Internal(ret.error());
    } else {
        out = *ret;
    }

    m_reply << *pack::json::serialize(out, pack::Option::PrettyPrint);

    return HTTP_OK;
}

} // namespace fty::disco

registerHandler(fty::disco::ConfigRead)
