#pragma once
#include <fty/expected.h>
#include <pack/pack.h>

namespace fty::disco {

static constexpr const char* ConfigFile = "/etc/fty-discovery-ng/fty-discovery-ng.cfg";

struct ConfigDiscovery : public pack::Node
{
    struct Server : public pack::Node
    {
        pack::Int64  timeout    = FIELD("timeout");
        pack::Int32  background = FIELD("background");
        pack::String workdir    = FIELD("workdir");
        pack::Int32  verbose    = FIELD("verbose");

        using pack::Node::Node;
        META(Server, timeout, background, workdir, verbose);
    };

    struct Discovery : public pack::Node
    {
        enum class Type
        {
            Unknown,
            Local,
            Ip,
            Multy,
            Full
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
        pack::StringList scans = FIELD("scans_disabled");
        pack::StringList ips   = FIELD("ips_disabled");

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
        pack::String src  = FIELD("src");
        pack::Int32  type = FIELD("type");

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

    struct DefaultValuesExt : public pack::Node
    {
        pack::String key   = FIELD("key");
        pack::String value = FIELD("value");

        using pack::Node::Node;
        META(DefaultValuesExt, key, value);
    };

    Server           server    = FIELD("server");
    Discovery        discovery = FIELD("discovery");
    Disabled         disabled  = FIELD("disabled");
    DefaultValuesAux aux       = FIELD("defaultValuesAux");

    pack::ObjectList<DefaultValuesLink> links = FIELD("defaultValuesLink");

    Parameters      parameters = FIELD("parameters");
    pack::ObjectList<DefaultValuesExt> ext = FIELD("defaultValuesExt");

    using pack::Node::Node;
    META(ConfigDiscovery, server, discovery, disabled, aux, links, parameters, ext);

    fty::Expected<void> save(const std::string& path = ConfigFile);
    fty::Expected<void> load(const std::string& path = ConfigFile);
};

std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Discovery::Type value);
std::istream& operator>>(std::istream& ss, ConfigDiscovery::Discovery::Type& value);

} // namespace fty::disco
