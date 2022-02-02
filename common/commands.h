#pragma once
#include "discovery-config.h"
#include <pack/pack.h>

namespace fty::disco {

// =====================================================================================================================

static constexpr const char* Channel = "discovery";

// =====================================================================================================================

namespace commands::protocols {
    static constexpr const char* Subject = "protocols";

    class In : public pack::Node
    {
    public:
        pack::String               address   = FIELD("address");
        ConfigDiscovery::Protocols protocols = FIELD("protocols"); // optional

    public:
        using pack::Node::Node;
        META(In, address, protocols);
    };

    class Return : public pack::Node
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

namespace commands {
    class CommonIn : public pack::Node
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
        META(CommonIn, address, protocol, port, settings);
    };
} // namespace commands

namespace commands::assets {
    static constexpr const char* Subject = "assets";

    using In = commands::CommonIn;

    using Ext = pack::ObjectList<pack::StringMap>;

    class Asset : public pack::Node
    {
    public:
        pack::String name    = FIELD("name");
        pack::String type    = FIELD("type");
        pack::String subtype = FIELD("sub_type");
        Ext          ext     = FIELD("ext");

    public:
        using pack::Node::Node;
        META(Asset, name, type, subtype, ext);
    };

    class Return : public pack::Node
    {
    public:
        pack::String            subAddress = FIELD("sub_address", "-1");
        Asset                   asset      = FIELD("asset");
        pack::ObjectList<Asset> sensors    = FIELD("sensors");

    public:
        using pack::Node::Node;
        META(Return, subAddress, asset, sensors);
    };

    using Out = pack::ObjectList<Return>;
} // namespace commands::assets

// =====================================================================================================================

namespace commands::config {
    namespace read {
        static constexpr const char* Subject = "config-read";

        using Out = ConfigDiscovery;
    } // namespace read

    namespace create {
        static constexpr const char* Subject = "config-create";

        using In = ConfigDiscovery;
    } // namespace create

} // namespace commands::config

// =====================================================================================================================

namespace commands::scan {
    namespace status {
        static constexpr const char* Subject = "scan-status";

        enum class Status
        {
            UNKNOWN,
            CANCELLED_BY_USER,
            TERMINATED,
            IN_PROGRESS
        };

        std::ostream& operator<<(std::ostream& ss, Status value);
        std::istream& operator>>(std::istream& ss, Status& value);

        class Out : public pack::Node
        {
        public:
            pack::Enum<Status> status         = FIELD("status");
            pack::String       progress       = FIELD("progress");
            pack::UInt32       discovered     = FIELD("discovered");
            pack::UInt32       ups            = FIELD("ups-discovered");
            pack::UInt32       epdu           = FIELD("epdu-discovered");
            pack::UInt32       sts            = FIELD("sts-discovered");
            pack::UInt32       sensors        = FIELD("sensors-discovered");
            pack::UInt32       numOfAddress   = FIELD("number-of-address");
            pack::UInt32       addressScanned = FIELD("address-scanned");

        public:
            using pack::Node::Node;
            META(Out, status, progress, discovered, ups, epdu, sts, sensors, numOfAddress, addressScanned);
        };
    } // namespace status

    struct Response : public pack::Node
    {
        enum class Status
        {
            SUCCESS,
            FAILURE,
            UNKNOWN
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
        using In  = ConfigDiscovery;

    } // namespace start
} // namespace commands::scan

// =====================================================================================================================

} // namespace fty::disco
