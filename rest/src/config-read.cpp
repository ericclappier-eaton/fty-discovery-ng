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
    
    commands::config::read::Out out;

    if (auto conf = ConfigDiscoveryManager::instance().load(); !conf) {
        throw rest::errors::Internal(conf.error());
    } else {
        out = *conf;
    }

    m_reply << *pack::json::serialize(out, pack::Option::WithDefaults | pack::Option::PrettyPrint);

    return HTTP_OK;
}

} // namespace fty::disco

registerHandler(fty::disco::ConfigRead)
