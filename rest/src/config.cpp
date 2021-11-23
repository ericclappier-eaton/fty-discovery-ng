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
        std::stringstream                ss(in.type.value());
        ConfigDiscovery::Discovery::Type tmpType;
        ss >> tmpType;
        cd.discovery.type = tmpType;
    }
    if (in.scans.hasValue()) {
        appendString(cd.discovery.scans, in.scans);
    }
    if (in.ips.hasValue()) {
        appendString(cd.discovery.ips, in.ips);
    }
    if (in.docs.hasValue()) {
        appendString(cd.discovery.documents, in.docs);
    }
    if (in.status.hasValue()) {
        cd.aux.status = in.status;
    }
    if (in.priority.hasValue()) {
        cd.aux.priority = fty::convert<pack::Int32>(in.priority);
    }
    if (in.parent.hasValue()) {
        cd.aux.parent = in.parent;
    }
    if (in.linkSrc.hasValue()) {
        cd.link.src = in.linkSrc;
    }
    if (in.scansDisabled.hasValue()) {
        cd.disabled.scans.clear();
        appendString(cd.disabled.scans, in.scansDisabled);
    }
    if (in.ipsDisabled.hasValue()) {
        cd.disabled.ips.clear();
        appendString(cd.disabled.ips, in.ipsDisabled);
    }
    if (in.protocols.hasValue()) {
        appendString(cd.discovery.protocols, in.protocols);
    }
    if (in.dumpPool.hasValue()) {
        cd.parameters.maxDumpPoolNumber = fty::convert<pack::Int32>(in.dumpPool);
    }
    if (in.scanPool.hasValue()) {
        cd.parameters.maxScanPoolNumber = fty::convert<pack::Int32>(in.scanPool);
    }
    if (in.scanTimeout.hasValue()) {
        cd.parameters.nutScannerTimeOut = fty::convert<pack::Int32>(in.scanTimeout);
    }
    if (in.dumpLooptime.hasValue()) {
        cd.parameters.dumpDataLoopTime = fty::convert<pack::Int32>(in.dumpLooptime);
    }

    if(auto ret = saveToFile(cd); !ret){
        throw rest::errors::Internal(ret.error());
    }
    // commands::config::Out out;

    // m_reply << *pack::json::serialize(out);

    return HTTP_OK;
}

} // namespace fty::disco::zproject

registerHandler(fty::disco::zproject::Config)
