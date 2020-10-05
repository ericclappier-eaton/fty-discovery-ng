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

static Expected<void> fetchFromSecurityWallet(const std::string& id, Process& nutProc)
{
    fty::SocketSyncClient secwSyncClient("/run/fty-security-wallet/secw.socket");
    auto                  client = secw::ConsumerAccessor(secwSyncClient);

    auto levelStr = [](secw::Snmpv3SecurityLevel lvl) -> Expected<std::string> {
        switch (lvl) {
            case secw::NO_AUTH_NO_PRIV:
                return std::string("noAuthNoPriv");
            case secw::AUTH_NO_PRIV:
                return std::string("authNoPriv");
            case secw::AUTH_PRIV:
                return std::string("authPriv");
            case secw::MAX_SECURITY_LEVEL:
                return unexpected("No privs defined");
        }
        return unexpected("No privs defined");
    };

    auto authProtStr = [](secw::Snmpv3AuthProtocol proc) -> Expected<std::string> {
        switch (proc) {
            case secw::MD5:
                return std::string("MD5");
            case secw::SHA:
                return std::string("SHA");
            case secw::MAX_AUTH_PROTOCOL:
                return unexpected("Wrong protocol");
        }
        return unexpected("Wrong protocol");
    };

    auto authPrivStr = [](secw::Snmpv3PrivProtocol proc) -> Expected<std::string> {
        switch (proc) {
            case secw::DES:
                return std::string("DES");
            case secw::AES:
                return std::string("AES");
            case secw::MAX_PRIV_PROTOCOL:
                return unexpected("Wrong protocol");
        }
        return unexpected("Wrong protocol");
    };

    try {
        auto secCred = client.getDocumentWithPrivateData("default", id);

        if (auto credV3 = secw::Snmpv3::tryToCast(secCred)) {
            log_debug("Init from wallet for snmp v3");
            nutProc.setEnvVar("SU_VAR_VERSION", "v3");
            if (auto lvl = levelStr(credV3->getSecurityLevel())) {
                nutProc.setEnvVar("SU_VAR_SECLEVEL", *lvl);
            }
            nutProc.setEnvVar("SU_VAR_SECNAME", credV3->getSecurityName());
            nutProc.setEnvVar("SU_VAR_AUTHPASSWD", credV3->getAuthPassword());
            nutProc.setEnvVar("SU_VAR_PRIVPASSWD", credV3->getPrivPassword());
            if (auto prot = authProtStr(credV3->getAuthProtocol())) {
                nutProc.setEnvVar("SU_VAR_AUTHPROT", *prot);
            }
            if (auto prot = authPrivStr(credV3->getPrivProtocol())) {
                nutProc.setEnvVar("SU_VAR_PRIVPROT", *prot);
            }
        } else if (auto credV1 = secw::Snmpv1::tryToCast(secCred)) {
            log_debug("Init from wallet for snmp v1");
            nutProc.setEnvVar("SU_VAR_VERSION", "v1");
            nutProc.setEnvVar("SU_VAR_COMMUNITY", credV1->getCommunityName());
        } else {
            return unexpected("Wrong wallet configuratoion");
        }
    } catch (const secw::SecwException& err) {
        return unexpected(err.what());
    }
    return {};
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
    if (cmd.protocol == "NUT_XML_PDC") {
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
    auto                  dri = driverName(m_cmd);
    if (!dri) {
        return unexpected(dri.error());
    } else {
        driver = std::filesystem::path("/lib/nut") / *dri;
    }

    uint32_t port = m_cmd.port;
    if (!m_cmd.port.hasValue()) {
        if (*dri == "netxml-ups") {
            port = 80;
        } else if (*dri == "snmp-ups") {
            port = 161;
        }
    }

    if (!port) {
        return unexpected("Port is not specified");
    }

    std::string address = fmt::format("{}:{}", m_cmd.address.value(), port);

    if (*dri == "netxml-ups") {
        if (address.find("http://") != 0) {
            address = "http://" + address;
        }
    }

    log_debug("connect address: %s", address.c_str());

    // clang-format off
    std::vector<std::string> args = {
        "-s", "discover",
        "-x", fmt::format("port={}", address),
        "-d", "1"
    };
    // clang-format on
    if (m_cmd.settings.timeout.hasValue() && *dri == "snmp-ups") {
        args.push_back("-x");
        args.push_back(fmt::format("snmp_timeout={}", m_cmd.settings.timeout));
    }

    Process proc(driver.string(), args);

    if (m_cmd.protocol == "NUT_SNMP") {
        if (!m_cmd.settings.mib.empty()) {
            proc.setEnvVar("SU_VAR_MIBS", fty::job::mapMibToLegacy(m_cmd.settings.mib));
        }

        if (!m_cmd.settings.credentialId.empty()) {
            if (auto ret = fetchFromSecurityWallet(m_cmd.settings.credentialId, proc); !ret) {
                return unexpected(ret.error());
            }
        } else if (!m_cmd.settings.community.empty()) {
            proc.setEnvVar("SU_VAR_COMMUNITY", m_cmd.settings.community);
        } else {
            return unexpected("Credentials or community is not set");
        }
    }

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

    log_debug(cnt.c_str());

    std::stringstream ss(cnt);
    for (std::string line; std::getline(ss, line);) {
        auto [key, value] = fty::split<std::string, std::string>(line, rex);
        tmpMap.emplace(key, value);
    }

    auto addAssetVal = [&](commands::assets::Return::Asset& asset, const std::string& nutKey, const std::string& val) {
        if (auto key = mapKey(nutKey); !key.empty()) {
            if (key == "device.type") {
                asset.subtype = val;
            }
            auto& ext = asset.ext.append();
            ext.append(key, val);
            ext.append("read_only", "true");
        }
    };

    int dcount = fty::convert<int>(value(tmpMap, "device.count"));
    if (dcount) {
        // daisychain
        for (int i = 0; i < dcount; ++i) {
            auto& asset      = map.append();
            asset.subAddress = std::to_string(i + 1);
            asset.asset.type = "device";

            std::string prefix = "device." + std::to_string(i + 1) + ".";
            for (const auto& p : tmpMap) {
                if (p.first.find(prefix) != 0) {
                    continue;
                }

                auto nutKey = p.first.substr(prefix.size());
                addAssetVal(asset.asset, nutKey, p.second);
            }
        }
    } else {
        auto& asset      = map.append();
        asset.subAddress = "";
        asset.asset.type = "device";

        for (const auto& p : tmpMap) {
            addAssetVal(asset.asset, p.first, p.second);
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
