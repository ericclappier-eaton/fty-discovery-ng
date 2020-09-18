#include "nut.h"
#include "src/config.h"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <fty/process.h>
#include <fty/split.h>
#include <fty_log.h>
#include <netdb.h>
#include <nutclient.h>
#include <regex>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "src/jobs/common.h"

namespace fty::protocol {

// =====================================================================================================================

template <typename T>
struct identify
{
    using type = T;
};

template <typename K, typename V>
static V value(
    const std::map<K, V>& map, const typename identify<K>::type& key, const typename identify<V>::type& def = {})
{
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return def;
}
// =====================================================================================================================

class NutProcess
{
public:
    NutProcess(const commands::assets::In& cmd);
    ~NutProcess();
    Expected<void> run(commands::assets::Out& map) const;

private:
    void        parseOutput(const std::string& cnt, commands::assets::Out& map) const;
    std::string mapKey(const std::string& key) const;

private:
    std::filesystem::path m_driver;
    commands::assets::In  m_cmd;
    std::filesystem::path m_root;
};

// =====================================================================================================================

fty::Expected<commands::assets::Out> properties(const commands::assets::In& cmd)
{
    commands::assets::Out map;

    NutProcess nut(cmd);
    if (auto ret = nut.run(map)) {
        return std::move(map);
    } else {
        return unexpected(ret.error());
    }
}

// =====================================================================================================================

static std::string driverName(const commands::assets::In& cmd)
{
    if (cmd.driver.hasValue()) {
        return cmd.driver;
    }
    if (cmd.mib.hasValue()) {
        if (fty::job::isSnmp(cmd.mib)) {
            return "snmp-ups";
        }
    }
    return "dummy-ups";
}

NutProcess::NutProcess(const commands::assets::In& cmd)
    : m_driver(std::filesystem::path("/usr/lib/nut") / driverName(cmd))
    , m_cmd(cmd)
{
    char tmpl[] = "/tmp/nutXXXXXX";
    m_root      = mkdtemp(tmpl);
}

NutProcess::~NutProcess()
{
    if (std::filesystem::exists(m_root)) {
        std::filesystem::remove_all(m_root);
    }
}

Expected<void> NutProcess::run(commands::assets::Out& map) const
{
    // clang-format off
    Process proc(m_driver.string(), {
        "-s", "discover",
        "-x", fmt::format("port={}:{}", m_cmd.address.value(), m_cmd.port.value()),
        "-x", fmt::format("community={}", m_cmd.community.value()),
        "-d", "1"
    });
    // clang-format on

    proc.setEnvVar("NUT_STATEPATH", m_root.string());
    proc.setEnvVar("MIBDIRS", Config::instance().mibDatabase);

    if (auto pid = proc.run()) {
        proc.wait();
        std::string out = proc.readAllStandardOutput();
        parseOutput(out, map);
    } else {
        return unexpected(pid.error());
    }
    return {};
}

void NutProcess::parseOutput(const std::string& cnt, commands::assets::Out& map) const
{
    static std::regex rex("([a-z0-9\\.]+)\\s*:\\s+(.*)");

    std::map<std::string, std::string> tmpMap;

    std::stringstream ss(cnt);
    for (std::string line; std::getline(ss, line);) {
        auto [key, value] = fty::split<std::string, std::string>(line, rex);
        tmpMap.emplace(key, value);
    }

    int dcount = fty::convert<int>(value(tmpMap, "device.count"));
    if (dcount) {
        // daisychain
        for (int i = 0; i < dcount; ++i) {
            auto& asset      = map.append();
            asset.subAddress = std::to_string(i + 1);

            std::string prefix = "device." + std::to_string(i + 1) + ".";
            for (const auto& p : tmpMap) {
                if (p.first.find(prefix) != 0) {
                    continue;
                }

                if (auto key = mapKey(p.first.substr(prefix.size())); !key.empty()) {
                    auto& val = asset.asset.ext.append();
                    val.append(key, p.second);
                    val.append("read_only", "true");
                }
            }
        }
    } else {
        auto& asset      = map.append();
        asset.subAddress = "";
        for (const auto& p : tmpMap) {
            if (auto key = mapKey(p.first); !key.empty()) {
                auto& val = asset.asset.ext.append();
                val.append(key, p.second);
                val.append("read_only", "true");
            }
        }
    }
}

struct Mappping : public pack::Node
{
    pack::StringMap physicsMapping   = FIELD("physicsMapping");
    pack::StringMap inventoryMapping = FIELD("inventoryMapping");

    using pack::Node::Node;
    META(Mappping, physicsMapping, inventoryMapping);
};

std::string NutProcess::mapKey(const std::string& key) const
{
    static Mappping mapping;
    if (!mapping.hasValue()) {
        std::ifstream fs("/usr/share/fty-common-nut/mapping.conf");
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

    static std::regex rex("(.*\\.)(\\d+)(\\..*)");
    static std::regex rerex("#");

    std::smatch matches;
    if (std::regex_match(key, matches, rex)) {
        std::string num = matches.str(2);

        std::string repl = std::regex_replace(key, rex, "$1#$3");
        if (mapping.physicsMapping.contains(repl)) {
            return std::regex_replace(mapping.physicsMapping[repl], rerex, num);
        }
        if (mapping.inventoryMapping.contains(repl)) {
            return std::regex_replace(mapping.inventoryMapping[repl], rerex, num);
        }
        return "";
    } else {
        if (mapping.physicsMapping.contains(key)) {
            return mapping.physicsMapping[key];
        }
        if (mapping.inventoryMapping.contains(key)) {
            return mapping.inventoryMapping[key];
        }
    }
    return {};
}

} // namespace fty::protocol
