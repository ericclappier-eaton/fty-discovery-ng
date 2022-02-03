#include "config-create.h"
#include "commands.h"
#include "discovery-config-manager.h"
#include "discovery-config.h"
#include <fty/rest/component.h>

namespace fty::disco {

unsigned ConfigCreate::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    commands::config::create::In in;
    if (auto ret = pack::json::deserialize(m_request.body(), in); !ret) {
        throw rest::errors::Internal(ret.error());
    }

    ConfigDiscoveryManager::instance().set(in);
    if (auto ret = ConfigDiscoveryManager::instance().save(); !ret) {
        throw rest::errors::Internal(ret.error());
    }
    return HTTP_OK;
}

} // namespace fty::disco

registerHandler(fty::disco::ConfigCreate)
