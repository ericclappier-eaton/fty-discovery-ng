#include "discovery.h"
#include "daemon.h"
#include "jobs/discover.h"
#include <fty/fty-log.h>
#include <fty/thread-pool.h>
#include <iostream>

namespace fty {

Discovery::Discovery(const std::string& config)
    : m_configPath(config)
{
    m_stopSlot.connect(Daemon::instance().stopEvent);
    m_loadConfigSlot.connect(Daemon::instance().loadConfigEvent);
}

void Discovery::doStop()
{
    stop();
}

bool Discovery::loadConfig()
{
    if (auto ret = pack::yaml::deserializeFile(m_configPath, m_config); !ret) {
        logError() << ret.error();
        return false;
    }
    return true;
}

void Discovery::init()
{
    ThreadPool::init();
    m_bus.init(m_config.actorName);

    m_bus.subsribe("discover", &Discovery::discover, this);
}

void Discovery::shutdown()
{
    stop();
    logDbg() << "stopping discovery agent";
    ThreadPool::stop();
}

int Discovery::run()
{
    stop.wait();
    return 0;
}

const Config& Discovery::config() const
{
    return m_config;
}

void Discovery::discover(const Message& msg)
{
    logInfo() << "Discovery: got message " << msg.dump();
    ThreadPool::pushWorker<job::Discover>(msg, m_bus);
}

void Discovery::configure(const Message& /*msg*/)
{
}

void Discovery::details(const Message& /*msg*/)
{
}


} // namespace fty
