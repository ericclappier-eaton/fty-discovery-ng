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
#include "commands.h"
#include "config.h"
#include "daemon.h"
#include "jobs/assets.h"
#include "jobs/mibs.h"
#include "jobs/protocols.h"
#include "jobs/scan-start.h"
#include "jobs/scan-status.h"
#include "jobs/scan-stop.h"
#include <fty/thread-pool.h>
#include <fty_log.h>

namespace fty::disco {

Discovery::Discovery(const std::string& config) : m_configPath(config)
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
    logDebug("Load config from '{}'", m_configPath);

    if (auto ret = pack::yaml::deserializeFile(m_configPath, Config::instance()); !ret) {
        logError(ret.error());
        return false;
    }
    return true;
}

Expected<void> Discovery::init()
{
    if (auto res = m_bus.init(Config::instance().actorName)) {
        if (auto sub = m_bus.subsribe(Channel, &Discovery::discover, this)) {
            return {};
        } else {
            return unexpected(sub.error());
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

void Discovery::discover(const disco::Message& msg)
{
    logDebug("Discovery: got message {}", msg.dump());
    logDebug("Payload: {}", msg.userData.asString());
    if (msg.meta.subject == commands::protocols::Subject) {
        m_pool.pushWorker<job::Protocols>(msg, m_bus);
    } else if (msg.meta.subject == commands::mibs::Subject) {
        m_pool.pushWorker<job::Mibs>(msg, m_bus);
    } else if (msg.meta.subject == commands::assets::Subject) {
        m_pool.pushWorker<job::Assets>(msg, m_bus);
    } else if (msg.meta.subject == disco::commands::scan::status::Subject) {
        m_pool.pushWorker<job::ScanStatus>(msg, m_bus, m_autoDiscovery);
    } else if (msg.meta.subject == disco::commands::scan::stop::Subject) {
        m_pool.pushWorker<job::ScanStop>(msg, m_bus, m_autoDiscovery);
    } else if (msg.meta.subject == disco::commands::scan::start::Subject) {
        m_pool.pushWorker<job::ScanStart>(msg, m_bus, m_autoDiscovery);
    } else {
        logError("Subject not handled {}", msg.meta.subject);
    }
}

// =====================================================================================================================

Config& Config::instance()
{
    static Config inst;
    return inst;
}

} // namespace fty::disco
