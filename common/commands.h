#pragma once
#include <pack/pack.h>

namespace fty {

// =====================================================================================================================

static constexpr const char* Channel = "discovery";

// =====================================================================================================================

namespace commands::protocols {
    static constexpr const char* Subject = "protocols";

    class In : public pack::Node
    {
    public:
        pack::String address = FIELD("address");

    public:
        using pack::Node::Node;
        META(In, address);
    };

    class Return: public pack::Node
    {
    public:
        pack::String protocol = FIELD("protocol");
        pack::UInt32 port     = FIELD("port");
        pack::Bool reachable  = FIELD("reachable");
    public:
        using pack::Node::Node;
        META(Return, protocol, port, reachable);
    };

    using Out = pack::ObjectList<Return>;
} // namespace commands::protocols

// =====================================================================================================================

namespace commands::mibs {
    static constexpr const char* Subject = "mibs";

    class In : public pack::Node
    {
    public:
        pack::String address      = FIELD("address");
        pack::UInt32 port         = FIELD("port", 161);
        pack::String credentialId = FIELD("secw_credential_id");
        pack::String community    = FIELD("community");
        pack::UInt32 timeout      = FIELD("timeout", 1000); // timeout in milliseconds

    public:
        using pack::Node::Node;
        META(In, address, port, credentialId, community, timeout);
    };

    using Out = pack::StringList;
} // namespace commands::mibs

// =====================================================================================================================

namespace commands::assets {
    static constexpr const char* Subject = "assets";

    class In : public pack::Node
    {
    public:
        class Settings : public pack::Node
        {
        public:
            pack::String credentialId = FIELD("secw_credential_id");
            pack::String mib          = FIELD("mib");
            pack::String community    = FIELD("community");
            pack::UInt32 timeout      = FIELD("timeout", 1000); // timeout in milliseconds
            pack::String username     = FIELD("username");
            pack::String password     = FIELD("password");

            using pack::Node::Node;
            META(Settings, credentialId, mib, community, timeout, username, password);
        };

    public:
        pack::String address  = FIELD("address");
        pack::String protocol = FIELD("protocol");
        pack::UInt32 port     = FIELD("port");
        Settings     settings = FIELD("protocol_settings");

    public:
        using pack::Node::Node;
        META(In, address, protocol, port, settings);
    };

    class Return : public pack::Node
    {
    public:
        class Asset : public pack::Node
        {
        public:
            // pack::String                      name    = FIELD("name");
            pack::String                      type    = FIELD("type");
            pack::String                      subtype = FIELD("sub_type");
            pack::ObjectList<pack::StringMap> ext     = FIELD("ext");

        public:
            using pack::Node::Node;
            META(Asset, type, subtype, ext);
        };

    public:
        pack::String subAddress = FIELD("sub_address", "-1");
        Asset        asset      = FIELD("asset");

    public:
        using pack::Node::Node;
        META(Return, subAddress, asset);
    };

    using Out = pack::ObjectList<Return>;
} // namespace commands::assets

// =====================================================================================================================
/*
namespace disco::commands::config {
    static constexpr const char* Subject = "config";

    class In : public pack::Node
    {
    public:
        pack::String type  = FIELD("FTY_DISCOVERY_TYPE");
        pack::String scans = FIELD("FTY_DISCOVERY_SCANS");
        pack::String scans = FIELD("FTY_DISCOVERY_IPS");
        pack::String scans = FIELD("FTY_DISCOVERY_DOCUMENTS");
        pack::String scans = FIELD("FTY_DISCOVERY_DEFAULT_VALUES_STATUS");
        pack::String scans = FIELD("FTY_DISCOVERY_DEFAULT_VALUES_PRIORITY");
        pack::String scans = FIELD("FTY_DISCOVERY_DEFAULT_VALUES_PARENT");
        pack::String scans = FIELD("FTY_DISCOVERY_DEFAULT_VALUES_LINK_SRC");
        pack::String scans = FIELD("FTY_DISCOVERY_SCANS_DISABLED");
        pack::String scans = FIELD("FTY_DISCOVERY_IPS_DISABLED");
        pack::String scans = FIELD("FTY_DISCOVERY_PROTOCOLS");

        pack::String scans = FIELD("FTY_DISCOVERY_DUMP_POOL");
        pack::String scans = FIELD("FTY_DISCOVERY_SCAN_POOL");
        pack::String scans = FIELD("FTY_DISCOVERY_SCAN_TIMEOUT");
        pack::String scans = FIELD("FTY_DISCOVERY_DUMP_LOOPTIME");

    public:
        using pack::Node::Node;
        META(In, address);
    };

    using Out = pack::StringList;

} // namespace disco::commands::config

// =====================================================================================================================

namespace disco::commands::scan {
    static constexpr const char* Subject = "scan";

    class In : public pack::Node
    {
    public:
        enum class Type
        {
            Local,
            IP,
            Multy,
            Full,
            Unknown
        };
        pack::String     linkSrc   = FIELD("linkSrc");
        pack::String     parent    = FIELD("parent");
        pack::Int32      priority  = FIELD("priority");
        pack::StringList documents = FIELD("documents");
        pack::StringList ips       = FIELD("ips");
        pack::Enum<Type> type      = FIELD("type");

    public:
        using pack::Node::Node;
        META(In, type);
    };

    class Out : public pack::Node
    {
    public:
        enum class Status
        {
            CancelledByUser = 1,
            Terminated      = 2,
            InProgress      = 3
        };
        pack::Enum<Status> status     = FIELD("status");
        pack::String       progress   = FIELD("progress");
        pack::Int64        discovered = FIELD("discovered");
        pack::Int64        ups        = FIELD("ups-discovered");
        pack::Int64        epdu       = FIELD("epdu-discovered");
        pack::Int64        sts        = FIELD("sts-discovered");
        pack::Int64        sensors    = FIELD("sensors-discovered");

    public:
        using pack::Node::Node;
        META(Out, type);
    };


    // using Out = pack::StringList;

} // namespace disco::commands::scan
*/
// =====================================================================================================================

} // namespace fty
