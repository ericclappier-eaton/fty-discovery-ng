#include "config.h"
#include "common/commands.h"
#include "common/discovery-config.h"
#include <fty/rest/component.h>

namespace fty::disco::zproject {

unsigned Config::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    commands::config::In in;
    if (auto ret = pack::json::deserialize(m_request.body(), in); !ret) {
        throw rest::errors::Internal(ret.error());
    }

    /// Here we need to convert to basic disco config
    ConfigDiscovery cd;

    auto appendString = [](pack::StringList& to, const auto& from) {
        for (const auto& f : from) {
            to.append(f);
        }
    };

    if (in.type.hasValue()) {
        std::stringstream ss(in.type.value());
        ss >> cd.discovery.type;
    } else if (in.scans.hasValue()) {
        appendString(cd.discovery.scans, in.scans);
    } else if (in.ips.hasValue()) {
        appendString(cd.discovery.ips, in.ips);
    } else if (in.docs.hasValue()) {
        appendString(cd.discovery.documents, in.docs);
    } else if (in.status.hasValue()) {
    } else if (in.priority.hasValue()) {
    } else if (in.parent.hasValue()) {
    } else if (in.linkSrc.hasValue()) {
    } else if (in.scansDisabled.hasValue()) {
        appendString(cd.disabled.scans, in.scansDisabled);

    } else if (in.ipsDisabled.hasValue()) {
        appendString(cd.disabled.ips, in.ipsDisabled);

    } else if (in.protocols.hasValue()) {
        appendString(cd.discovery.protocols, in.protocols);
    } else if (in.dumpPool.hasValue()) {
    } else if (in.scanPool.hasValue()) {
    } else if (in.scanTimeout.hasValue()) {
    } else if (in.dumpLooptime.hasValue()) {
    }


    // commands::config::Out out;

    // m_reply << *pack::json::serialize(out);

    return HTTP_OK;
}

} // namespace fty::disco::zproject

registerHandler(fty::disco::zproject::Config)
