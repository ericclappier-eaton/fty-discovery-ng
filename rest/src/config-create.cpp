#include "config-create.h"
#include "commands.h"
#include "discovery-config.h"
#include <fty/rest/component.h>

namespace fty::disco {

unsigned ConfigCreate::run()
{
    rest::User user(m_request);
    if (auto ret = checkPermissions(user.profile(), m_permissions); !ret) {
        throw rest::Error(ret.error());
    }

    commands::config::In in;
    if (auto ret = pack::json::deserialize(m_request.body(), in); !ret) {
        throw rest::errors::Internal(ret.error());
    }

    ConfigDiscovery cd;
    if (auto ret = cd.load(); !ret) {
        throw rest::errors::Internal(ret.error());
    }

    if (in.type.hasValue()) {
        std::stringstream                ss(in.type.value());
        ConfigDiscovery::Discovery::Type tmpType;
        ss >> tmpType;
        cd.discovery.type = tmpType;
    }
    if (in.scans.hasValue()) {
        cd.discovery.scans = in.scans;
    }
    if (in.ips.hasValue()) {
        cd.discovery.ips = in.ips;
    }
    if (in.docs.hasValue()) {
        cd.discovery.documents = in.docs;
    }
    if (in.status.hasValue()) {
        cd.aux.status = in.status;
    }
    if (in.priority.hasValue()) {
        cd.aux.priority = in.priority;
    }
    if (in.parent.hasValue()) {
        cd.aux.parent = in.parent;
    }
    if (in.links.hasValue()) {
        for(const auto & link : in.links){
            auto & tmp = cd.links.append();
            tmp.src = link;
            tmp.type = 1;
        }
    }
    if (in.scansDisabled.hasValue()) {
        cd.disabled.scans = in.scansDisabled;
    }
    if (in.ipsDisabled.hasValue()) {
        cd.disabled.ips = in.ipsDisabled;
    }
    if (in.protocols.hasValue()) {
        cd.discovery.protocols = in.protocols;
    }
    if (in.dumpPool.hasValue()) {
        cd.parameters.maxDumpPoolNumber = in.dumpPool;
    }
    if (in.scanPool.hasValue()) {
        cd.parameters.maxScanPoolNumber = in.scanPool;
    }
    if (in.scanTimeout.hasValue()) {
        cd.parameters.nutScannerTimeOut = in.scanTimeout;
    }
    if (in.dumpLooptime.hasValue()) {
        cd.parameters.dumpDataLoopTime = in.dumpLooptime;
    }

    if (auto ret = cd.save(); !ret) {
        throw rest::errors::Internal(ret.error());
    }
    return HTTP_OK;
}

} // namespace fty::disco

registerHandler(fty::disco::ConfigCreate)
