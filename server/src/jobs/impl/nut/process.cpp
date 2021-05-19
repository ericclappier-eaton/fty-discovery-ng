#include "process.h"
#include "src/config.h"
#include "src/jobs/impl/mibs.h"
#include <filesystem>
#include <fty/process.h>
#include <fty_security_wallet.h>
#include <unistd.h>

namespace fty::impl::nut {
Process::Process(const std::string& protocol)
    : m_protocol(protocol)
{
    char tmpl[] = "/tmp/nutXXXXXX";
    if (auto temp = mkdtemp(tmpl)) {
        m_root = temp;
    } else {
        m_root = "/tmp/nut";
    }
}

Process::~Process()
{
    if (std::filesystem::exists(m_root)) {
        std::filesystem::remove_all(m_root);
    }
}

Expected<void> Process::setupSnmp(const std::string& address, uint16_t port)
{
    if (!port) {
        port = 161;
    }

    std::string toconnect = fmt::format("{}:{}", address, port);

    if (auto path = findExecutable("snmp-ups")) {
        // clang-format off
        m_process = std::unique_ptr<fty::Process>(new fty::Process(*path, {
            "-s", "discover",
            "-x", fmt::format("port={}", toconnect),
            "-d", "1"
        }));
        // clang-format on
        m_process->setEnvVar("NUT_STATEPATH", m_root);
        m_process->setEnvVar("MIBDIRS", Config::instance().mibDatabase);
        return {};
    } else {
        return unexpected(path.error());
    }
}

Expected<void> Process::setupXmlPdc(const std::string& address, uint16_t port)
{
    if (!port) {
        port = 80;
    }

    std::string toconnect = fmt::format("{}:{}", address, port);

    if (address.find("http://") != 0) {
        toconnect = "http://" + toconnect;
    }

    if (auto path = findExecutable("netxml-ups")) {
        // clang-format off
        m_process = std::unique_ptr<fty::Process>(new fty::Process(*path, {
            "-s", "discover",
            "-x", fmt::format("port={}", toconnect),
            "-d", "2"
        }));
        // clang-format on
        m_process->setEnvVar("NUT_STATEPATH", m_root);
        return {};
    } else {
        return unexpected(path.error());
    }
}

Expected<void> Process::setupPowercom(const std::string& address)
{
    if (auto path = findExecutable("etn-nut-powerconnect")) {
        // clang-format off
        m_process = std::unique_ptr<fty::Process>(new fty::Process(*path, {
            "-x", fmt::format("port={}", address),
            "-d", "2"
        }));
        // clang-format on
        return {};
    } else {
        return unexpected(path.error());
    }
}

Expected<void> Process::init(const std::string& address, uint16_t port)
{
    std::string driver;
    if (m_protocol == "nut_snmp") {
        if (auto ret = setupSnmp(address, port); !ret) {
            return unexpected(ret.error());
        }
    } else if (m_protocol == "nut_xml_pdc") {
        if (auto ret = setupXmlPdc(address, port); !ret) {
            return unexpected(ret.error());
        }
    } else if (m_protocol == "nut_powercom") {
        if (auto ret = setupPowercom(address); !ret) {
            return unexpected(ret.error());
        }
    } else {
        return unexpected("Protocol {} is not supported", m_protocol);
    }

    return {};
}

Expected<std::string> Process::findExecutable(const std::string& name) const
{
    static std::vector<std::filesystem::path> paths = {"/usr/lib/nut", "/lib/nut"};

    for (const auto& path : paths) {
        auto check = path / name;
        if (std::filesystem::exists(check) && access(check.c_str(), X_OK) == 0) {
            return (path / name).string();
        }
    }

    return unexpected("Executable {} was not found", name);
}

Expected<void> Process::setCredentialId(const std::string& credential)
{
    if (!m_process) {
        return unexpected("uninitialized");
    }

    if (m_protocol == "nut_snmp") {
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
            auto secCred = client.getDocumentWithPrivateData("default", credential);

            if (auto credV3 = secw::Snmpv3::tryToCast(secCred)) {
                log_debug("Init from wallet for snmp v3");
                m_process->setEnvVar("SU_VAR_VERSION", "v3");
                if (auto lvl = levelStr(credV3->getSecurityLevel())) {
                    m_process->setEnvVar("SU_VAR_SECLEVEL", *lvl);
                }
                m_process->setEnvVar("SU_VAR_SECNAME", credV3->getSecurityName());
                m_process->setEnvVar("SU_VAR_AUTHPASSWD", credV3->getAuthPassword());
                m_process->setEnvVar("SU_VAR_PRIVPASSWD", credV3->getPrivPassword());
                if (auto prot = authProtStr(credV3->getAuthProtocol())) {
                    m_process->setEnvVar("SU_VAR_AUTHPROT", *prot);
                }
                if (auto prot = authPrivStr(credV3->getPrivProtocol())) {
                    m_process->setEnvVar("SU_VAR_PRIVPROT", *prot);
                }
            } else if (auto credV1 = secw::Snmpv1::tryToCast(secCred)) {
                log_debug("Init from wallet for snmp v1");
                setCommunity(credV1->getCommunityName());
            } else {
                return unexpected("Wrong wallet configuratoion");
            }
        } catch (const secw::SecwException& err) {
            return unexpected(err.what());
        }
    } else if (m_protocol == "nut_powercom") {
        fty::SocketSyncClient secwSyncClient("/run/fty-security-wallet/secw.socket");
        auto                  client = secw::ConsumerAccessor(secwSyncClient);

        try {
            auto secCred = client.getDocumentWithPrivateData("default", credential);
            if (auto cred = secw::UserAndPassword::tryToCast(secCred)) {
                m_process->addArgument("-x");
                m_process->addArgument(fmt::format("username={}", cred->getUsername()));

                m_process->addArgument("-x");
                m_process->addArgument(fmt::format("password={}", cred->getPassword()));
            }
        } catch (const std::runtime_error& err) {
            return unexpected(err.what());
        } catch (const secw::SecwException& err) {
            return unexpected(err.what());
        }
    }
    return {};
}

Expected<void> Process::setCredential(const std::string& userName, const std::string& password)
{
    if (m_protocol == "nut_powercom") {
        m_process->addArgument("-x");
        m_process->addArgument(fmt::format("username={}", userName));

        m_process->addArgument("-x");
        m_process->addArgument(fmt::format("password={}", password));
    }
    return {};
}

Expected<void> Process::setCommunity(const std::string& community)
{
    if (!m_process) {
        return unexpected("uninitialized");
    }

    if (m_protocol != "nut_snmp") {
        return unexpected("unsupported");
    }

    m_process->setEnvVar("SU_VAR_VERSION", "v1");
    m_process->setEnvVar("SU_VAR_COMMUNITY", community);
    m_process->addArgument("-x");
    m_process->addArgument(fmt::format("community={}", community));
    return {};
}

Expected<void> Process::setTimeout(uint milliseconds)
{
    if (!m_process) {
        return unexpected("uninitialized");
    }

    if (m_protocol != "nut_snmp") {
        return unexpected("unsupported");
    }

    m_process->setEnvVar("SU_VAR_TIMEOUT", std::to_string(milliseconds / 1000));
    m_process->addArgument("-x");
    m_process->addArgument(fmt::format("snmp_timeout={}", milliseconds / 1000));
    return {};
}

Expected<void> Process::setMib(const std::string& mib)
{
    if (!m_process) {
        return unexpected("uninitialized");
    }

    if (m_protocol != "nut_snmp") {
        return unexpected("unsupported");
    }

    log_debug("Mib: %s, legacy name: %s", mib.c_str(), mapMibToLegacy(mib).c_str());
    m_process->setEnvVar("SU_VAR_MIBS", mapMibToLegacy(mib));
    return {};
}

Expected<std::string> Process::run() const
{
    if (auto pid = m_process->run()) {
        if (auto stat = m_process->wait(); *stat == 0) {
            return m_process->readAllStandardOutput();
        } else {
            std::string stdError = m_process->readAllStandardError();
            // workaround with nut_powercom: Test first if the credentials are correct
            if (m_protocol == "nut_powercom" &&
                stdError.find("Error when get client token on") != std::string::npos) {
                return unexpected("Bad login or password");
            }
            else {
                return unexpected(stdError);
            }
        }
    } else {
        log_error("Run error: %s", pid.error().c_str());
        return unexpected(pid.error());
    }
}

} // namespace fty::impl::nut
