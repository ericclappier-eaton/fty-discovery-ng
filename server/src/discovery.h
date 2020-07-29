/*  =========================================================================
    discovery.h - Discovery message dispatcher, worker

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

#pragma once
#include "message-bus.h"
#include <fty/event.h>
#include <string>

namespace fty {

/// Discovery message dispatcher, worker
class Discovery
{
public:
    explicit Discovery(const std::string& config);

    /// Loads config
    bool loadConfig();

    /// Runs message dispatcher
    int  run();

    /// Inits discovery instance
    void init();

    /// Shutdowns discovery instance
    void shutdown();

    /// Events to stop discovery message dispatcher
    Event<> stop;

private:
    void discover(const Message& msg);
    void doStop();

private:
    std::string m_configPath;
    MessageBus  m_bus;

    Slot<>               m_stopSlot       = {&Discovery::doStop, this};
    Slot<>               m_loadConfigSlot = {&Discovery::loadConfig, this};
    Slot<const Message&> m_discoverSlot   = {&Discovery::discover, this};
};

} // namespace fty
