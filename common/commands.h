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
        enum class Available
        {
            Unknown, // unknown if protocol is available/not available
            No,      // protocol is not available
            Maybe,   // don't know if protocol is available/not available
            Yes      // protocol is available
        };

        pack::String          protocol  = FIELD("protocol");
        pack::UInt32          port      = FIELD("port");
        pack::Enum<Available> available = FIELD("available");
        pack::Bool            reachable = FIELD("reachable");

    public:
        using pack::Node::Node;
        META(Return, protocol, port, reachable, available);
    };

    inline std::ostream& operator<<(std::ostream& ss, Return::Available value)
    {
        ss << [&]() {
            switch (value) {
                case Return::Available::Unknown:
                    return "unknown";
                case Return::Available::No:
                    return "no";
                case Return::Available::Maybe:
                    return "maybe";
                case Return::Available::Yes:
                    return "yes";
            }
            return "unknown";
        }();
        return ss;
    };

    inline std::istream& operator>>(std::istream& ss, Return::Available& value)
    {
        std::string strval;
        ss >> strval;
        if (strval == "unknown") {
            value = Return::Available::Unknown;
        } else if (strval == "no") {
            value = Return::Available::No;
        } else if (strval == "maybe") {
            value = Return::Available::Maybe;
        } else if (strval == "yes") {
            value = Return::Available::Yes;
        } else {
            value = Return::Available::Unknown;
        }
        return ss;
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

namespace disco::commands::config {
    static constexpr const char* Subject = "config";

    class In : public pack::Node
    {
    public:
        pack::String     type          = FIELD("FTY_DISCOVERY_TYPE");
        pack::StringList scans         = FIELD("FTY_DISCOVERY_SCANS");
        pack::StringList ips           = FIELD("FTY_DISCOVERY_IPS");
        pack::StringList docs          = FIELD("FTY_DISCOVERY_DOCUMENTS");
        pack::String     status        = FIELD("FTY_DISCOVERY_DEFAULT_VALUES_STATUS");
        pack::Int32      priority      = FIELD("FTY_DISCOVERY_DEFAULT_VALUES_PRIORITY");
        pack::String     parent        = FIELD("FTY_DISCOVERY_DEFAULT_VALUES_PARENT");
        pack::StringList links         = FIELD("FTY_DISCOVERY_DEFAULT_VALUES_LINK_SRC");
        pack::StringList scansDisabled = FIELD("FTY_DISCOVERY_SCANS_DISABLED");
        pack::StringList ipsDisabled   = FIELD("FTY_DISCOVERY_IPS_DISABLED");
        pack::StringList protocols     = FIELD("FTY_DISCOVERY_PROTOCOLS");
        pack::Int32      dumpPool      = FIELD("FTY_DISCOVERY_DUMP_POOL");
        pack::Int32      scanPool      = FIELD("FTY_DISCOVERY_SCAN_POOL");
        pack::Int32      scanTimeout   = FIELD("FTY_DISCOVERY_SCAN_TIMEOUT");
        pack::Int32      dumpLooptime  = FIELD("FTY_DISCOVERY_DUMP_LOOPTIME");

    public:
        using pack::Node::Node;
        META(In, type, scans, ips, docs, status, priority, parent, links, scansDisabled, ipsDisabled, protocols,
            dumpPool, scanPool, scanTimeout, dumpLooptime);
    };

} // namespace disco::commands::config

// =====================================================================================================================

namespace disco::commands::scan {
    namespace status {
        static constexpr const char* Subject = "scan-status";

        class Out : public pack::Node
        {
        public:
            enum class Status
            {
                Unknown,
                CancelledByUser,
                Terminated,
                InProgress
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
            META(Out, status, progress, discovered, ups, epdu, sts, sensors);
        };
        std::ostream& operator<<(std::ostream& ss, Out::Status value);
        std::istream& operator>>(std::istream& ss, Out::Status& value);

    } // namespace status

    struct Response : public pack::Node
    {
        enum class Status
        {
            Success,
            Fail,
            Unknown
        };

        pack::Enum<Status> status = FIELD("status");
        pack::String       data   = FIELD("data");

    public:
        using pack::Node::Node;
        META(Response, status, data);
    };

    std::ostream& operator<<(std::ostream& ss, Response::Status value);
    std::istream& operator>>(std::istream& ss, Response::Status& value);

    namespace stop {
        static constexpr const char* Subject = "scan-stop";

        using Out = Response;
    } // namespace stop

    namespace start {
        static constexpr const char* Subject = "scan-start";

        using Out = Response;

        class In : public pack::Node
        {
        public:
            enum class Type
            {
                Unknown,
                Local,
                Ip,
                Multy,
                Full
            };
            pack::StringList linkSrc       = FIELD("linkSrc");
            pack::String     parent        = FIELD("parent");
            pack::Int32      priority      = FIELD("priority");
            pack::StringList documents     = FIELD("documents");
            pack::StringList ips           = FIELD("ips");
            pack::StringList ipsDisabled   = FIELD("ipsDisabled");
            pack::StringList scans         = FIELD("scans");
            pack::StringList scansDisabled = FIELD("scansDisabled");
            pack::Enum<Type> type          = FIELD("type");

        public:
            using pack::Node::Node;
            META(In, linkSrc, parent, priority, documents, ips, ipsDisabled, scans, scansDisabled, type);
        };

        std::ostream& operator<<(std::ostream& ss, In::Type value);
        std::istream& operator>>(std::istream& ss, In::Type& value);

    } // namespace start
} // namespace disco::commands::scan

// =====================================================================================================================

} // namespace fty
