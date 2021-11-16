#include "discovery-config.h"
#include <map>
// #include "fty/convert.h"

namespace fty::disco {

ConfigDiscovery& ConfigDiscovery::operator+=(const zproject::Argument& arg)
{
    auto var = arg.first;
    if(var == zproject::Type){
        std::stringstream ss(arg.second);
        auto tt = ConfigDiscovery::Discovery::Type::Unknown;
        ss >> tt;
        discovery.type = tt;
    } else if(var == zproject::Scans) {
        discovery.scans.append(arg.second);
    }
    else if(var == zproject::Ips) {
        discovery.ips.append(arg.second);
    }
    else if(var == zproject::Documents) {
        discovery.documents.append(arg.second); 
    }
    else if(var == zproject::Protocols) {
        discovery.protocols.append(arg.second);
    }

    else if(var == zproject::DefaultValStatus) {
        aux.status = arg.second;
    }
    else if(var == zproject::DefaultValPriority) {
        aux.priority = fty::convert<pack::Int32>(arg.second);
    }
    else if(var == zproject::DefaultValParent) {
        aux.parent = arg.second;
    }

    else if(var == zproject::DefaultValLinkSrc) {
        links.append().src = arg.second;
    }

    else if(var == zproject::ScansDisabled) {
        disabled.scans
    }
    else if(var == zproject::IpsDisabled) {}

    else if(var == zproject::DumpPool) {}
    else if(var == zproject::ScanPool) {}
    else if(var == zproject::ScanTimeout) {}
    else if(var == zproject::DumpLooptime) {}

    std::map<zproject::Variable, pack::Attribute&> variables =
        // clang-format off
    {
        /* { zproject::Type, this->discovery.type}, 
        { zproject::Scans, this->discovery.scans }, 
        { zproject::Ips, this->discovery.ips },
        { zproject::Documents, this->discovery.documents },
        { zproject::Protocols, this->discovery.protocols },

        { zproject::DefaultValStatus, this->aux.status },
        { zproject::DefaultValPriority, this->aux.priority },
        { zproject::DefaultValParent, this->aux.parent }, 

        { zproject::DefaultValLinkSrc, this->links[0].src },*/

        { zproject::ScansDisabled, this->disabled.scans },
        { zproject::IpsDisabled, this->disabled.ips },

        { zproject::DumpPool, this->parameters.maxDumpPoolNumber },
        { zproject::ScanPool, this->parameters.maxScanPoolNumber },
        { zproject::ScanTimeout, this->parameters.nutScannerTimeOut },
        { zproject::DumpLooptime, this->parameters.dumpDataLoopTime },
    };
    // clang-format on

    parameters.dumpDataLoopTime.typeName();
    auto &var = variables[arg.first];
    // var.ThisType
    // var = fty::convert<var.>(arg.second);

    return *this;
}


std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Discovery::Type value)
{
    using Type = fty::disco::ConfigDiscovery::Discovery::Type;

    ss << [&]() {
        switch (value) {
            case Type::Local:
                return "localscan";
            case Type::Ip:
                return "ipscan";
            case Type::Multy:
                return "multiscan";
            case Type::Full:
                return "fullscan";
            case Type::Unknown:
                return "UNKNOWN";
        }
        return "Unknown";
    }();
    return ss;
}

std::istream& operator>>(std::istream& ss, ConfigDiscovery::Discovery::Type& value)
{
    using Type = fty::disco::ConfigDiscovery::Discovery::Type;

    std::string strval;
    ss >> strval;
    if (strval == "localscan") {
        value = Type::Local;
    } else if (strval == "ipscan") {
        value = Type::Ip;
    } else if (strval == "multiscan") {
        value = Type::Multy;
    } else if (strval == "fullscan") {
        value = Type::Full;
    } else {
        value = Type::Unknown;
    }
    return ss;
}

} // namespace fty::disco
