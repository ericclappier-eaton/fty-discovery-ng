#pragma once
#include <fty/rest-support.h>
#include <pack/pack.h>

namespace fty {

class Protocols : public rest::Runner
{
public:
    static constexpr const char* NAME = "discovery_protocols_get";

public:
    using rest::Runner::Runner;
    unsigned run() override;

private:
    Expected<pack::StringList> protocols(const std::string& address);

private:
    // clang-format off
    Permissions m_permissions = {
        { BiosProfile::Admin, rest::Access::Read }
    };
    // clang-format on
};

}
