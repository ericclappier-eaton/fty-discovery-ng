#pragma once
#include <fty/expected.h>
#include <pack/pack.h>

namespace fty::disco {

static constexpr const char* ConfigFile = "/etc/fty-discovery-ng/config-discovery.conf";

struct ConfigDiscovery : public pack::Node
{
    struct Protocol : public pack::Node
    {
    public:
        enum class Type
        {
            Unknown,
            Powercom,
            XmlPdc,
            Snmp
        };

        pack::Enum<Type> protocol = FIELD("protocol");
        pack::UInt32List ports    = FIELD("ports");

    public:
        using pack::Node::Node;
        META(Protocol, protocol, ports);
    };

    using Protocols = pack::ObjectList<Protocol>;

    struct Discovery : public pack::Node
    {
        enum class Type
        {
            Unknown,
            Local,
            Ip,
            Multi,
            Full
        };

        pack::Enum<Type> type      = FIELD("type");
        pack::StringList scans     = FIELD("scans");
        pack::StringList ips       = FIELD("ips");
        pack::StringList documents = FIELD("documents");
        Protocols        protocols = FIELD("protocols");

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
        enum CreateMode
        {
            CreateModeOneAsset = 1,
            CreateModeCsv
        };

        enum class Priority
        {
            P1 = 1,
            P2,
            P3,
            P4,
            P5
        };

        pack::UInt32 createMode       = FIELD("create_mode", CreateMode::CreateModeOneAsset);
        pack::String createUser       = FIELD("create_user", "");  // usual
        pack::String parent           = FIELD("parent");
        pack::Enum<Priority> priority = FIELD("priority");
        pack::String status           = FIELD("status");

        using pack::Node::Node;
        META(DefaultValuesAux, createMode, createUser, parent, priority, status);
    };

    struct DefaultValuesLink : public pack::Node
    {
        pack::String src  = FIELD("src");
        pack::UInt32 type = FIELD("type");

        using pack::Node::Node;
        META(DefaultValuesLink, src, type);
    };

    struct DefaultValuesExt : public pack::Node
    {
        pack::String key   = FIELD("key");
        pack::String value = FIELD("value");

        using pack::Node::Node;
        META(DefaultValuesExt, key, value);
    };

    Discovery        discovery = FIELD("discovery");
    Disabled         disabled  = FIELD("disabled");
    DefaultValuesAux aux       = FIELD("defaultValuesAux");

    pack::ObjectList<DefaultValuesLink> links = FIELD("defaultValuesLink");

    pack::ObjectList<DefaultValuesExt> ext = FIELD("defaultValuesExt");

    pack::Bool recoverFqdn = FIELD("recoverFqdn", true); 


    using pack::Node::Node;
    META(ConfigDiscovery, discovery, disabled, aux, links, ext, recoverFqdn);

    fty::Expected<void> save(const std::string& path = ConfigFile);
    fty::Expected<void> load(const std::string& path = ConfigFile);
};

std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Protocol::Type value);
std::istream& operator>>(std::istream& ss, ConfigDiscovery::Protocol::Type& value);

std::ostream& operator<<(std::ostream& ss, ConfigDiscovery::Discovery::Type value);
std::istream& operator>>(std::istream& ss, ConfigDiscovery::Discovery::Type& value);

} // namespace fty::disco
