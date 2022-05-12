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

    void set(const ConfigDiscovery& config);

private:
    ConfigDiscoveryManager() = default;

    std::optional<ConfigDiscovery> m_config = nullptr;

    bool validateIp(const std::string& ip);
};

} // namespace fty::disco
