#include "discovery-config-manager.h"

namespace fty::disco {

ConfigDiscoveryManager& ConfigDiscoveryManager::instance()
{
    static ConfigDiscoveryManager inst;
    return inst;
}

fty::Expected<void> ConfigDiscoveryManager::save(const std::string& path)
{
    if (!m_config.has_value()) {
        return fty::unexpected("discovery config was not loaded => nothing to save");
    }
    m_config.value().save(path);

    return {};
}

fty::Expected<ConfigDiscovery> ConfigDiscoveryManager::load(const std::string& path)
{
    ConfigDiscovery tmp;
    if (auto ret = tmp.load(path); !ret) {
        return fty::unexpected(ret.error());
    }
    m_config = tmp;

    return *m_config;
}

fty::Expected<ConfigDiscovery> ConfigDiscoveryManager::config() const
{
    if (!m_config.has_value()) {
        return fty::unexpected("discovery config was not loaded");
    }
    return *m_config;
}

void ConfigDiscoveryManager::set(const ConfigDiscovery& config)
{
    m_config = config;
}

bool ConfigDiscoveryManager::validateIp(const std::string& ip)
{
    static auto regex =
        std::regex("(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(ip, regex);
}


} // namespace fty::disco
