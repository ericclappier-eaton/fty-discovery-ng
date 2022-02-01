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
            //pack::String                      name    = FIELD("name");
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

} // namespace fty
