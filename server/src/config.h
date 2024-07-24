/*  =========================================================================
    config.h - Discovery config data desctripor

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
#include <pack/pack.h>

namespace fty::disco {

class Config : public pack::Node
{
public:
    pack::String actorName      = FIELD("actor-name", "fty-discovery-ng");
    pack::String mibDatabase    = FIELD("mib-database", "mibs");
    pack::Bool   tryAll         = FIELD("try-all", false);
    pack::UInt32 pollScanMax    = FIELD("poll-scan-max", 50);
    pack::String endpoint       = FIELD("endpoint", "ipc://@/malamute");
    pack::String assetAgentName = FIELD("asset-agent-name", "asset-agent-ng");
    pack::String assetQueueName = FIELD("asset-queue-name", "FTY.Q.ASSET.QUERY");

public:
    using pack::Node::Node;
    META(Config, actorName, mibDatabase, tryAll, pollScanMax, endpoint, assetAgentName, assetQueueName);

public:
    static Config& instance();
};

} // namespace fty::disco
