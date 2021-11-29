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
    }
    if (out.scans.key() == *key) {
        out.scans = cd.discovery.scans;
    }
    if (out.ips.key() == *key) {
        out.ips = cd.discovery.ips;
    }
    if (out.docs.key() == *key) {
        out.docs = cd.discovery.documents;
    }
    if (out.status.key() == *key) {
        out.status = cd.aux.status;
    }
    if (out.priority.key() == *key) {
        out.priority = cd.aux.priority;
    }
    if (out.parent.key() == *key) {
        out.parent = cd.aux.parent;
    }
    if (out.linkSrc.key() == *key) {
        out.linkSrc = cd.link.src;
    }
    if (out.scansDisabled.key() == *key) {
        out.scansDisabled = cd.disabled.scans;
    }
    if (out.ipsDisabled.key() == *key) {
        out.ipsDisabled = cd.disabled.ips;
    }
    if (out.protocols.key() == *key) {
        out.protocols = cd.discovery.protocols;
    }
    if (out.dumpPool.key() == *key) {
        out.dumpPool = cd.parameters.maxDumpPoolNumber;
    }
    if (out.scanPool.key() == *key) {
        out.scanPool = cd.parameters.maxScanPoolNumber;
    }
    if (out.scanTimeout.key() == *key) {
        out.scanTimeout = cd.parameters.nutScannerTimeOut;
    }
    if (out.dumpLooptime.key() == *key) {
        out.dumpLooptime = cd.parameters.dumpDataLoopTime;
    }

    m_reply << *pack::json::serialize(out, pack::Option::PrettyPrint);

    return HTTP_OK;
}

} // namespace fty::disco

registerHandler(fty::disco::ConfigRead)
