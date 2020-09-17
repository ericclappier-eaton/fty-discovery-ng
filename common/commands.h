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

    public:
        using pack::Node::Node;
        META(In, address, port, credentialId);
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

    public:
        using pack::Node::Node;
        META(In, address, port, credentialId);
    };

    class Out : public pack::Node
    {
    public:
        pack::String ip           = FIELD("ip");
        pack::String uuid         = FIELD("uuid");
        pack::String hostName     = FIELD("hostname");
        pack::String manufacturer = FIELD("manufacturer");
        pack::String model        = FIELD("model");

    public:
        using pack::Node::Node;
        META(Out, ip, uuid, hostName, manufacturer, model);
    };

} // namespace commands::assets

// =====================================================================================================================

} // namespace fty
