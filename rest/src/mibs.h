#pragma once

#include <fty/rest-support.h>
#include <pack/pack.h>

namespace fty {

class Mibs : public rest::Runner
{
public:
    static constexpr const char* NAME = "discovery_mibs_get";

public:
    using rest::Runner::Runner;
    unsigned run() override;

private:
    Expected<pack::StringList> mibs(const std::string& address, uint32_t port, const std::string& secw);

private:
    // clang-format off
    Permissions m_permissions = {
        { BiosProfile::Admin, rest::Access::Read }
    };
    // clang-format on
};

} // namespace fty
