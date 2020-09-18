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

    using Out = pack::StringList;

} // namespace commands::protocols

// =====================================================================================================================

namespace commands::mibs {
    static constexpr const char* Subject = "mibs";

    class In : public pack::Node
    {
    public:
        pack::String address      = FIELD("address");
        pack::UInt32 port         = FIELD("port", 161);
        pack::String credentialId = FIELD("credential-id");
        pack::String community    = FIELD("community");

    public:
        using pack::Node::Node;
        META(In, address, port, credentialId, community);
    };

    using Out = pack::StringList;
} // namespace commands::mibs

// =====================================================================================================================

namespace commands::assets {
    static constexpr const char* Subject = "assets";

    class In : public pack::Node
    {
    public:
        pack::String address      = FIELD("address");
        pack::UInt32 port         = FIELD("port", 161);
        pack::String credentialId = FIELD("credential-id");
        pack::String community    = FIELD("community");
        pack::String driver       = FIELD("driver");
        pack::String mib          = FIELD("mib");

    public:
        using pack::Node::Node;
        META(In, address, port, credentialId, community, driver, mib);
    };

    class Return : public pack::Node
    {
    public:
        class Asset : public pack::Node
        {
        public:
            pack::ObjectList<pack::StringMap> ext = FIELD("ext");

        public:
            using pack::Node::Node;
            META(Asset, ext);
        };

    public:
        pack::String subAddress = FIELD("sub_address");
        Asset        asset      = FIELD("asset");

    public:
        using pack::Node::Node;
        META(Return, subAddress, asset);
    };

    using Out = pack::ObjectList<Return>;
} // namespace commands::assets

// =====================================================================================================================

} // namespace fty
