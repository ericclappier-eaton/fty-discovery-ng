/*  =========================================================================
    discovery.cpp - Discovery message dispatcher, worker

    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
 */

#include "discovery.h"
#include "config.h"
#include "daemon.h"
#include "jobs/chaineddevices.h"
#include "jobs/configure.h"
#include "jobs/discover.h"
#include <fty/fty-log.h>
#include <fty/thread-pool.h>

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
    if (auto ret = pack::yaml::deserializeFile(m_configPath, Config::instance()); !ret) {
        logError() << ret.error();
        return false;
    }
    return true;
}

void Discovery::init()
{
    ThreadPool::init();
    m_bus.init(Config::instance().actorName);
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

void Discovery::discover(const Message& msg)
{
    logInfo() << "Discovery: got message " << msg.dump();
    if (msg.meta.subject == "discovery") {
        ThreadPool::pushWorker<job::Discover>(msg, m_bus);
    } else if (msg.meta.subject == "configure") {
        ThreadPool::pushWorker<job::Configure>(msg, m_bus);
    } else if (msg.meta.subject == "details") {
        ThreadPool::pushWorker<job::ChainedDevices>(msg, m_bus);
    }
}

// =====================================================================================================================

Config& Config::instance()
{
    static Config inst;
    return inst;
}

} // namespace fty
