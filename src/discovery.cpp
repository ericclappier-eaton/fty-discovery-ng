#include "discovery.h"
#include "daemon.h"
#include <fty/fty-log.h>
#include <signal.h>
#include <unistd.h>
#include <fty/fty-log.h>
#include <iostream>

namespace fty {

Discovery::Discovery(const std::string& config)
    : m_configPath(config)
{
    m_stopSlot.connect(Daemon::instance().stopEvent);
    m_loadConfigSlot.connect(Daemon::instance().loadConfigEvent);
}

void Discovery::stop()
{
    logDbg() << "stopping discovery agent";
}

bool Discovery::loadConfig()
{
    pack::yaml::deserializeFile(m_configPath, m_config);
    std::cerr << m_config.logConfig.value() << std::endl;
    return true;
}

int Discovery::run()
{
    return 0;
}

const Config& Discovery::config() const
{
    return m_config;
}

} // namespace fty
