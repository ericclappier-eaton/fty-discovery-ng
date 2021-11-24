#pragma once
#include <fty/expected.h>
#include <pack/pack.h>

namespace fty::disco {

static constexpr const char* ConfigFile = "/etc/fty-discovery-ng/fty-discovery-ng.cfg";
struct ConfigDiscovery;
namespace zproject {
    using Variable = std::string;
    using Value    = std::string;

    using Argument = std::pair<Variable, Value>;

    static constexpr const char* ZConfigFile = "/etc/fty-discovery/fty-discovery-ng.conf";

    static constexpr const char* Type      = "FTY_DISCOVERY_TYPE";
    static constexpr const char* Scans     = "FTY_DISCOVERY_SCANS";
    static constexpr const char* Ips       = "FTY_DISCOVERY_IPS";
    static constexpr const char* Documents = "FTY_DISCOVERY_DOCUMENTS";
    static constexpr const char* Protocols = "FTY_DISCOVERY_PROTOCOLS";

    static constexpr const char* DefaultValStatus   = "FTY_DISCOVERY_DEFAULT_VALUES_STATUS";
    static constexpr const char* DefaultValPriority = "FTY_DISCOVERY_DEFAULT_VALUES_PRIORITY";
    static constexpr const char* DefaultValParent   = "FTY_DISCOVERY_DEFAULT_VALUES_PARENT";
    static constexpr const char* DefaultValLinkSrc  = "FTY_DISCOVERY_DEFAULT_VALUES_LINK_SRC";

    static constexpr const char* ScansDisabled = "FTY_DISCOVERY_SCANS_DISABLED";
    static constexpr const char* IpsDisabled   = "FTY_DISCOVERY_IPS_DISABLED";

    static constexpr const char* DumpPool     = "FTY_DISCOVERY_DUMP_POOL";
    static constexpr const char* ScanPool     = "FTY_DISCOVERY_SCAN_POOL";
    static constexpr const char* ScanTimeout  = "FTY_DISCOVERY_SCAN_TIMEOUT";
    static constexpr const char* DumpLooptime = "FTY_DISCOVERY_DUMP_LOOPTIME";

    fty::Expected<void> saveToFile(const fty::disco::ConfigDiscovery& config, const std::string& path = ZConfigFile);

} // namespace zproject

struct ConfigDiscovery : public pack::Node
{
    struct Server : public pack::Node
    {
        pack::Int64  timeout    = FIELD("timeout");
        pack::Int32  background = FIELD("background"); // bool?
        pack::String workdir    = FIELD("workdir");
        pack::Int32  verbose    = FIELD("verbose"); // bool?

        using pack::Node::Node;
        META(Server, timeout, background, workdir, verbose);
    };

    struct Discovery : public pack::Node
    {
        enum class Type
        {
            Local,
            Ip,
            Multy,
            Full,
            Unknown
        };

        pack::Enum<Type> type      = FIELD("type");
        pack::StringList scans     = FIELD("scans");
        pack::StringList ips       = FIELD("ips");
        pack::StringList documents = FIELD("documents");
        pack::StringList protocols = FIELD("protocols");


        using pack::Node::Node;
        META(Discovery, type, scans, ips, documents, protocols);
    };

    struct Disabled : public pack::Node
    {
        pack::StringList scans = FIELD("scans_disabled"); // bool?
        pack::StringList ips   = FIELD("ips_disabled");   // bool?

        using pack::Node::Node;
        META(Disabled, scans, ips);
    };

    struct DefaultValuesAux : public pack::Node
    {
        pack::Int32  createMode = FIELD("create_mode");
        pack::String createUser = FIELD("create_user");
        pack::String parent     = FIELD("parent");
        pack::Int32  priority   = FIELD("priority");
        pack::String status     = FIELD("status"); // enum?

        using pack::Node::Node;
        META(DefaultValuesAux, createMode, createUser, parent, priority, status);
    };

    struct DefaultValuesLink : public pack::Node
    {
        pack::String src  = FIELD("src");  // string??
        pack::Int32  type = FIELD("type"); // int?

        using pack::Node::Node;
        META(DefaultValuesLink, src, type);
    };

    struct Parameters : public pack::Node
    {
        pack::String mappingFile       = FIELD("mappingFile");
        pack::Int32  maxDumpPoolNumber = FIELD("maxDumpPoolNumber");
        pack::Int32  maxScanPoolNumber = FIELD("maxScanPoolNumber");
        pack::Int32  nutScannerTimeOut = FIELD("nutScannerTimeOut");
        pack::Int32  dumpDataLoopTime  = FIELD("dumpDataLoopTime");

        using pack::Node::Node;
        META(Parameters, mappingFile, maxDumpPoolNumber, maxScanPoolNumber, nutScannerTimeOut, dumpDataLoopTime);
    };

    struct Log : public pack::Node
    {
        pack::String config = FIELD("config");

        using pack::Node::Node;
        META(Log, config);
    };

    Server           server    = FIELD("server");
    Discovery        discovery = FIELD("discovery");
    Disabled         disabled  = FIELD("disabled");
    DefaultValuesAux aux       = FIELD("defaultValuesAux");

    DefaultValuesLink link = FIELD("defaultValuesLink");

    Parameters parameters = FIELD("parameters");
    Log        log        = FIELD("log");


    using pack::Node::Node;
    META(ConfigDiscovery, server, discovery, disabled, aux, link, parameters, log);

    fty::Expected<void> save(const std::string& path = ConfigFile);
    fty::Expected<void> load(const std::string& path = ConfigFile);
};

std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Discovery::Type value);
std::istream& operator>>(std::istream& ss, ConfigDiscovery::Discovery::Type& value);

} // namespace fty::disco
