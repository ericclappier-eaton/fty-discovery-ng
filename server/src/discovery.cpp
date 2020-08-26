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
#include "commands.h"
#include "jobs/chaineddevices.h"
#include "jobs/configure.h"
#include "jobs/discover.h"
#include <fty_log.h>
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
        log_error(ret.error().c_str());
        return false;
    }
    return true;
}

Expected<void> Discovery::init()
{
    if (auto res = m_bus.init(Config::instance().actorName)) {
        if (auto sub = m_bus.subsribe(fty::Channel, &Discovery::discover, this)) {
            return {};
        } else {
            return unexpected(res.error());
        }
    } else {
        return unexpected(res.error());
    }
}

void Discovery::shutdown()
{
    stop();
    m_pool.stop();
}

int Discovery::run()
{
    stop.wait();
    return 0;
}

void Discovery::discover(const Message& msg)
{
    log_info("Discovery: got message %s", msg.dump().c_str());
    if (msg.meta.subject == commands::protocols::Subject) {
        m_pool.pushWorker<job::Discover>(msg, m_bus);
    } else if (msg.meta.subject == commands::mibs::Subject) {
        m_pool.pushWorker<job::Configure>(msg, m_bus);
//    } else if (msg.meta.subject == "details") {
//        ThreadPool::pushWorker<job::ChainedDevices>(msg, m_bus);
    }
}

// =====================================================================================================================

Config& Config::instance()
{
    static Config inst;
    return inst;
}

} // namespace fty
