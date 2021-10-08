/*  ====================================================================================================================
    Copyright (C) 2020 Eaton
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
    ====================================================================================================================
*/

#include "mapper.h"
#include <fstream>
#include <fty/string-utils.h>
#include <fty_log.h>
#include <pack/pack.h>
#include <regex>


namespace fty::impl::nut {

// =====================================================================================================================

struct Mappping : public pack::Node
{
    pack::StringMap physicsMapping   = FIELD("physicsMapping");
    pack::StringMap inventoryMapping = FIELD("inventoryMapping");

    using pack::Node::Node;
    META(Mappping, physicsMapping, inventoryMapping);

    std::string map(const std::string& key) const
    {
        //We do not need physics mapping for discovery
        /*if (physicsMapping.contains(key)) {
            return physicsMapping[key];
        }*/ 
        if (inventoryMapping.contains(key)) {
            return inventoryMapping[key];
        }
        return {};
    }
};

// =====================================================================================================================

std::string Mapper::mapKey(const std::string& key)
{
    static std::regex rerex("#");
    static std::regex rex("(.*\\.)(\\d+)(\\..*)");

    auto&       map = mapping();
    std::smatch matches;
    if (std::regex_match(key, matches, rex)) {
        std::string num  = matches.str(2);
        std::string repl = std::regex_replace(key, rex, "$1#$3");

        if (auto mapped = map.map(repl); !mapped.empty()) {
            return std::regex_replace(mapped, rerex, num);
        }
    }
    return map.map(key);
}

const Mappping& Mapper::mapping()
{
    static Mappping mapping;
    if (!mapping.hasValue()) {
        std::ifstream fs(mapFile);
        std::string   mapCnt;
        // This config IS JSON WITH CPP COMMENTS! Remove it :(
        for (std::string line; std::getline(fs, line);) {
            if (auto tr = trimmed(line); tr.find("//") == 0 || tr.find("/*") == 0) {
                continue;
            }
            mapCnt += line + "\n";
        }
        if (auto res = pack::json::deserialize(mapCnt, mapping); !res) {
            log_error(res.error().c_str());
        }
    }
    return mapping;
}

// =====================================================================================================================

} // namespace fty::protocol::nut
