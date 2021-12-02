#pragma once
#include "commands.h"
#include "discovery-config.h"
#include <fty/expected.h>
#include <optional>
#include <pack/pack.h>


namespace fty::disco {
class ConfigDiscoveryManager
{
public:
    static ConfigDiscoveryManager& instance();
    fty::Expected<void>            save(const std::string& path = ConfigFile);
    fty::Expected<ConfigDiscovery> load(const std::string& path = ConfigFile);
    fty::Expected<ConfigDiscovery> config() const;

    fty::Expected<void>                     commandCreate(const commands::config::Config& in);
    fty::Expected<commands::config::Config> commandRead(const pack::StringList& keys) const;

private:
    ConfigDiscoveryManager() = default;

    std::optional<ConfigDiscovery> m_config = nullptr;
};

} // namespace fty::disco
