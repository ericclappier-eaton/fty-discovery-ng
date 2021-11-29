#include "config-read.h"
#include "commands.h"
#include "discovery-config.h"
#include <fty/rest/component.h>

namespace fty::disco {

unsigned ConfigRead::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    auto key = m_request.queryArg<std::string>("key");
    if (!key) {
        throw rest::errors::RequestParamRequired("key");
    }

    ConfigDiscovery cd;
    if (auto ret = cd.load(); !ret) {
        throw rest::errors::Internal(ret.error());
    }

    commands::config::In out;

    if (out.type.key() == *key) {
        std::stringstream ss;
        ss << cd.discovery.type;
        out.type = ss.str();
    } else if (out.scans.key() == *key) {
        out.scans = cd.discovery.scans;
    } else if (out.ips.key() == *key) {
        out.ips = cd.discovery.ips;
    } else if (out.docs.key() == *key) {
        out.docs = cd.discovery.documents;
    } else if (out.status.key() == *key) {
        out.status = cd.aux.status;
    } else if (out.priority.key() == *key) {
        out.priority = cd.aux.priority;
    } else if (out.parent.key() == *key) {
        out.parent = cd.aux.parent;
    } else if (out.links.key() == *key) {
        for (const auto& link : cd.links) {
            out.links.append(link.src);
        }
    } else if (out.scansDisabled.key() == *key) {
        out.scansDisabled = cd.disabled.scans;
    } else if (out.ipsDisabled.key() == *key) {
        out.ipsDisabled = cd.disabled.ips;
    } else if (out.protocols.key() == *key) {
        out.protocols = cd.discovery.protocols;
    } else if (out.dumpPool.key() == *key) {
        out.dumpPool = cd.parameters.maxDumpPoolNumber;
    } else if (out.scanPool.key() == *key) {
        out.scanPool = cd.parameters.maxScanPoolNumber;
    } else if (out.scanTimeout.key() == *key) {
        out.scanTimeout = cd.parameters.nutScannerTimeOut;
    } else if (out.dumpLooptime.key() == *key) {
        out.dumpLooptime = cd.parameters.dumpDataLoopTime;
    }

    m_reply << *pack::json::serialize(out, pack::Option::PrettyPrint);

    return HTTP_OK;
}

} // namespace fty::disco

registerHandler(fty::disco::ConfigRead)
