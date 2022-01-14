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

fty::Expected<void> ConfigDiscoveryManager::commandCreate(const commands::config::Config& in)
{
    ConfigDiscovery tmp;

    if (!m_config.has_value()) {
        if (auto ret = load(); !ret) {
            return fty::unexpected(ret.error());
        } else {
            tmp = *ret;
        }
    }

    if (in.type.hasValue()) {

        std::stringstream                ss(in.type.value());
        ConfigDiscovery::Discovery::Type tmpType;
        ss >> tmpType;
        tmp.discovery.type = tmpType;
    }
    if (in.scans.hasValue()) {
        tmp.discovery.scans = in.scans;
    }
    if (in.ips.hasValue()) {
        for (const auto& ip : in.ips) {
            if (!validateIp(ip)) {
                return fty::unexpected("invalid ip: {}", ip);
            }
        }
        tmp.discovery.ips = in.ips;
    }
    if (in.docs.hasValue()) {
        tmp.discovery.documents = in.docs;
    }
    if (in.status.hasValue()) {
        tmp.aux.status = in.status;
    }
    if (in.priority.hasValue()) {
        tmp.aux.priority = in.priority;
    }
    if (in.parent.hasValue()) {
        tmp.aux.parent = in.parent;
    }
    if (in.links.hasValue()) {
        for (const auto& link : in.links) {
            auto& appended = tmp.links.append();
            appended.src   = link;
            appended.type  = 1;
        }
    }
    if (in.scansDisabled.hasValue()) {
        tmp.disabled.scans = in.scansDisabled;
    }
    if (in.ipsDisabled.hasValue()) {
        tmp.disabled.ips = in.ipsDisabled;
    }
    if (in.protocols.hasValue()) {
        tmp.discovery.protocols = in.protocols;
    }
    if (in.dumpPool.hasValue()) {
        tmp.parameters.maxDumpPoolNumber = in.dumpPool;
    }
    if (in.scanPool.hasValue()) {
        tmp.parameters.maxScanPoolNumber = in.scanPool;
    }
    if (in.scanTimeout.hasValue()) {
        tmp.parameters.nutScannerTimeOut = in.scanTimeout;
    }
    if (in.dumpLooptime.hasValue()) {
        tmp.parameters.dumpDataLoopTime = in.dumpLooptime;
    }

    m_config = tmp;

    return {};
}

fty::Expected<commands::config::Config> ConfigDiscoveryManager::commandRead(const pack::StringList& keys) const
{
    if (!m_config.has_value()) {
        return fty::unexpected("config not loaded => cannot resolve read");
    }

    commands::config::Config out;

    for (const auto& key : keys) {
        if (auto ret = commandReadKey(key, out); !ret) {
            return fty::unexpected(ret.error());
        }
    }
    return out;
}

fty::Expected<void> ConfigDiscoveryManager::commandReadKey(const std::string& key, commands::config::Config& out) const
{
    if (!m_config.has_value()) {
        return fty::unexpected("config not loaded => cannot resolve read");
    }
    const auto& conf = *m_config;

    if (out.type.key() == key) {
        std::stringstream ss;
        ss << conf.discovery.type;
        out.type = ss.str();
    } else if (out.scans.key() == key) {
        out.scans = conf.discovery.scans;
    } else if (out.ips.key() == key) {
        out.ips = conf.discovery.ips;
    } else if (out.docs.key() == key) {
        out.docs = conf.discovery.documents;
    } else if (out.status.key() == key) {
        out.status = conf.aux.status;
    } else if (out.priority.key() == key) {
        out.priority = conf.aux.priority;
    } else if (out.parent.key() == key) {
        out.parent = conf.aux.parent;
    } else if (out.links.key() == key) {
        for (const auto& link : conf.links) {
            out.links.append(link.src);
        }
    } else if (out.scansDisabled.key() == key) {
        out.scansDisabled = conf.disabled.scans;
    } else if (out.ipsDisabled.key() == key) {
        out.ipsDisabled = conf.disabled.ips;
    } else if (out.protocols.key() == key) {
        out.protocols = conf.discovery.protocols;
    } else if (out.dumpPool.key() == key) {
        out.dumpPool = conf.parameters.maxDumpPoolNumber;
    } else if (out.scanPool.key() == key) {
        out.scanPool = conf.parameters.maxScanPoolNumber;
    } else if (out.scanTimeout.key() == key) {
        out.scanTimeout = conf.parameters.nutScannerTimeOut;
    } else if (out.dumpLooptime.key() == key) {
        out.dumpLooptime = conf.parameters.dumpDataLoopTime;
    } else {
        return fty::unexpected("key does not exist: {}", key);
    }

    return {};
}

bool ConfigDiscoveryManager::validateIp(const std::string& ip)
{
    static auto regex =
        std::regex("(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(ip, regex);
}


} // namespace fty::disco
