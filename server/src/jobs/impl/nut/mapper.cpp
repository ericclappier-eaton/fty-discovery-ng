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


namespace fty::disco::impl::nut {

// =====================================================================================================================

struct Mapping : public pack::Node
{
    pack::StringMap physicsMapping         = FIELD("physicsMapping");
    pack::StringMap inventoryMapping       = FIELD("inventoryMapping");
    pack::StringMap sensorInventoryMapping = FIELD("sensorInventoryMapping");

    using pack::Node::Node;
    META(Mapping, physicsMapping, inventoryMapping, sensorInventoryMapping);

    std::string map(const std::string& key) const
    {
        if (inventoryMapping.contains(key)) {
            return inventoryMapping[key];
        }
        if (sensorInventoryMapping.contains(key)) {
            return sensorInventoryMapping[key];
        }
        return {};
    }
};

// =====================================================================================================================

std::string Mapper::mapKey(const std::string& key, int index)
{
    // Need to capture #n:
    // device.n.xxx - daisychain device
    // device.n.ambient.xxx  -> emp01 with daisychain (only one sensor on a device #n)
    // ambient.n.xxx  -> emp02 with no daisychain
    // device.1.ambient.n.xxx -> emp02 with daisychain (all sensor are on master #1)
    const static std::regex prefixRegex(
        R"xxx(.*((?:device(?!\.\d*\.ambient\.\d*\.))|ambient)\.(\d*)\.(.+))xxx", std::regex::optimize);
    std::smatch matches;

    std::string transformedKey = key;

    //logTrace("Looking for key {} (index {})", key, index);
    auto map = mapping();

    // Daisy-chained or indexed sensor special case, need to fold it back into conventional case.
    if (index > 0 && std::regex_match(key, matches, prefixRegex)) {
        //logTrace("match1 = {}, match2 = {}, match3 = {}", matches.str(1), matches.str(2), matches.str(3));
        if (matches.str(2) == std::to_string(index)) {
            //logTrace("key {} (index {}) 1-> found and test {}.{}", key, index, matches.str(1), matches.str(3));
            // We have a "{device,ambient}.<id>.<property>" property, map it to either device.<property> or
            // ambient.<property> or <property> (for device only)
            if (!map.map(matches.str(1) + "." + matches.str(3)).empty()) {
                transformedKey = matches.str(1) + "." + matches.str(3);
                // logTrace("key {} (index {}) -> {}", key, index, transformedKey);
            } else {
                if (matches.str(1) != "ambient") {
                    transformedKey = matches.str(3);
                    // logTrace("key {} (index {}) -> {}", key, index, transformedKey);
                }
            }
        } else {
            // Not the daisy-chained or sensor index we're looking for.
            transformedKey = "";
        }
    }
    return transformedKey.empty() ? "" : map.map(transformedKey);
}

const Mapping& Mapper::mapping()
{
    static Mapping mapping;
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

} // namespace fty::disco::impl::nut
