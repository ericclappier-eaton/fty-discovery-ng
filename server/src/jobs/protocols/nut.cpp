#include "nut.h"
#include "src/config.h"
#include "src/jobs/common.h"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <fty/process.h>
#include <fty/split.h>
#include <fty_log.h>
#include <fty_security_wallet.h>
#include <regex>

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

static void fetchFromSecurityWallet(const std::string& id, std::vector<std::string>& cmdLine)
{
    fty::SocketSyncClient secwSyncClient("/run/fty-security-wallet/secw.socket");
    auto                  client = secw::ConsumerAccessor(secwSyncClient);

    auto levelStr = [](secw::Snmpv3SecurityLevel lvl) -> Expected<std::string> {
        switch (lvl) {
        case secw::NO_AUTH_NO_PRIV: return std::string("noAuthNoPriv");
        case secw::AUTH_NO_PRIV: return std::string("authNoPriv");
        case secw::AUTH_PRIV: return std::string("authPriv");
        case secw::MAX_SECURITY_LEVEL: return unexpected("No privs defined");
        }
        return unexpected("No privs defined");
    };

    auto authProtStr = [](secw::Snmpv3AuthProtocol proc) -> Expected<std::string> {
        switch (proc) {
        case secw::MD5: return std::string("MD5");
        case secw::SHA: return std::string("SHA");
        case secw::MAX_AUTH_PROTOCOL: return unexpected("Wrong protocol");
        }
        return unexpected("Wrong protocol");
    };

    auto authPrivStr = [](secw::Snmpv3PrivProtocol proc) -> Expected<std::string> {
        switch (proc) {
        case secw::DES: return std::string("DES");
        case secw::AES: return std::string("AES");
        case secw::MAX_PRIV_PROTOCOL: return unexpected("Wrong protocol");
        }
        return unexpected("Wrong protocol");
    };


    auto secCred = client.getDocumentWithPrivateData("default", id);
    auto credV3  = secw::Snmpv3::tryToCast(secCred);
    auto credV1  = secw::Snmpv1::tryToCast(secCred);
    credV1->getCommunityName();
    if (credV3) {
        cmdLine.push_back("-x");
        cmdLine.push_back("snmp_version=v3");
        if (auto lvl = levelStr(credV3->getSecurityLevel())) {
            cmdLine.push_back("-x");
            cmdLine.push_back(fmt::format("secLevel={}", *lvl));
        }
        cmdLine.push_back("-x");
        cmdLine.push_back(fmt::format("secName={}", credV3->getSecurityName()));
        cmdLine.push_back("-x");
        cmdLine.push_back(fmt::format("authPassword={}", credV3->getAuthPassword()));
        cmdLine.push_back("-x");
        cmdLine.push_back(fmt::format("privPassword={}", credV3->getPrivPassword()));
        if (auto prot = authProtStr(credV3->getAuthProtocol())) {
            cmdLine.push_back("-x");
            cmdLine.push_back(fmt::format("authProtocol={}", *prot));
        }
        if (auto prot = authPrivStr(credV3->getPrivProtocol())){
            cmdLine.push_back("-x");
            cmdLine.push_back(fmt::format("privProtocol={}", *prot));
        }
    } else if (credV1) {
        cmdLine.push_back("-x");
        cmdLine.push_back(fmt::format("community={}", credV1->getCommunityName()));
        cmdLine.push_back("-x");
        cmdLine.push_back("snmp_version=v1");
    }
}

// =====================================================================================================================


static Expected<std::string> driverName(const commands::assets::In& cmd)
{
    if (cmd.protocol == "NUT_SNMP") {
        if (!cmd.settings.mib.empty() && !fty::job::isSnmp(cmd.settings.mib)) {
            log_error("Mib %s possible is not supported by snmp-ups", cmd.settings.mib.value().c_str());
        }
        return std::string("snmp-ups");
    }
    if (cmd.protocol == "XML_PDC") {
        return std::string("netxml-ups");
    }
    return unexpected("Protocol {} is not supported", cmd.protocol.value());
}

NutProcess::NutProcess(const commands::assets::In& cmd)
    : m_cmd(cmd)
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
    std::filesystem::path driver;
    if (auto dri = driverName(m_cmd); !dri) {
        return unexpected(dri.error());
    } else {
        driver = std::filesystem::path("/lib/nut") / *dri;
    }

    // clang-format off
    std::vector<std::string> cmdLine = {
        "-s", "discover",
        "-x", fmt::format("port={}:{}", m_cmd.address.value(), m_cmd.port.value()),
        "-d", "1"
    };
    // clang-format on

    if (m_cmd.protocol == "NUT_SNMP") {
        if (!m_cmd.settings.mib.empty()) {
            cmdLine.push_back("-x");
            cmdLine.push_back(fmt::format("mibs={}", fty::job::mapMibToLegacy(m_cmd.settings.mib)));
        }

        if (!m_cmd.settings.credentialId.empty()) {
            fetchFromSecurityWallet(m_cmd.settings.credentialId, cmdLine);
        } else if (!m_cmd.settings.community.empty()) {
            cmdLine.push_back("-x");
            cmdLine.push_back(fmt::format("community={}", m_cmd.settings.community.value()));
        } else {
            return unexpected("Credentials or community is not set");
        }
    }

    for(const auto& it: cmdLine) {
        log_info("--- %s", it.c_str());
    }

    Process proc(driver.string(), cmdLine);
    proc.setEnvVar("NUT_STATEPATH", m_root.string());
    if (m_cmd.protocol == "NUT_SNMP") {
        proc.setEnvVar("MIBDIRS", Config::instance().mibDatabase);
    }

    if (auto pid = proc.run()) {
        if (auto stat = proc.wait(); *stat == 0) {
            parseOutput(proc.readAllStandardOutput(), map);
        } else {
            log_debug(proc.readAllStandardOutput().c_str());
            return unexpected(proc.readAllStandardError());
        }
    } else {
        log_error("Run error: %s", pid.error().c_str());
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
