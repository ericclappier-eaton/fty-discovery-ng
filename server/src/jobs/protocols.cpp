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

#include "protocols.h"
#include "impl/mibs.h"
#include "impl/ping.h"
#include "impl/xml-pdc.h"
#include "impl/snmp.h"
#include <fty/string-utils.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <set>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

namespace fty::disco::job {

// =====================================================================================================================

enum class Type
{
    Powercom = 1,
    Xml      = 2,
    Snmp     = 3,
};

// =====================================================================================================================

inline std::ostream& operator<<(std::ostream& ss, Type type)
{
    switch (type) {
        case Type::Powercom:
            ss << "GenApi";
            break;
        case Type::Xml:
            ss << "Xml";
            break;
        case Type::Snmp:
            ss << "Snmp";
            break;
        default:
            ss << "protocol-type-unknown";
    }
    return ss;
}

// =====================================================================================================================
std::string Protocols::getProtocolStr(const Type type)
{
    std::string protocol;
    switch (type) {
        case Type::Snmp:
            protocol = "nut_snmp";
            break;
        case Type::Xml:
            protocol = "nut_xml_pdc";
            break;
        case Type::Powercom:
            protocol = "nut_powercom";
            break;
    }
    return protocol;
}

// =====================================================================================================================
Expected<commands::protocols::Out> Protocols::getProtocols(const commands::protocols::In& in) const
{
    if (!available(in.address)) {
        return unexpected("Host is not available: {}", in.address.value());
    }

    commands::protocols::Out out;
    // supported protocols, tokenized with default port
    // in *order* of preferences
    struct {
        Type protocol;
        std::string protocolStr;
        uint16_t port;
    } tries[] = {
        {Type::Powercom, "nut_powercom", 443},
        {Type::Xml, "nut_xml_pdc", 80},
        {Type::Snmp, "nut_snmp", 161},
    };

    for (auto& aux : tries) {
        auto& protocol = out.append();
        protocol.protocol = aux.protocolStr;
        protocol.port = aux.port;
        protocol.reachable = false; // default, not reachable

        // try to reach server
        switch (aux.protocol) {
            case Type::Powercom: {
                if (auto res = tryPowercom(in, aux.port)) {
                    logInfo("Found Powercom device on port {}", aux.port);
                    protocol.reachable = true; // port is reachable
                }
                else {
                    logInfo("Skipped GenApi/{}, reason: {}", aux.port, res.error());
                }
                break;
            }
            case Type::Xml: {
                if (auto res = tryXmlPdc(in, aux.port)) {
                    logInfo("Found XML device on port {}", aux.port);
                    protocol.reachable = true; // port is reachable
                }
                else {
                    logInfo("Skipped xml_pdc/{}, reason: {}", aux.port, res.error());
                }
                break;
            }
            case Type::Snmp: {
                if (auto res = trySnmp(in, aux.port)) {
                    logInfo("Found SNMP device on port {}", aux.port);
                    protocol.reachable = true; // port is reachable
                }
                else {
                    logInfo("Skipped SNMP/{}, reason: {}", aux.port, res.error());
                }
                break;
            }
            default:
                logError("protocol not handled (type: {})", aux.protocol);
        }
    }

    return out;
}

void Protocols::run(const commands::protocols::In& in, commands::protocols::Out& out)
{
    if (in.address == "__fake__") {
        out.setValue({"nut_snmp", "nut_xml_pdc"});
        return;
    }

    auto protocols = getProtocols(in);
    if (!protocols) {
        throw Error(protocols.error().c_str());
    }
    out = *protocols;
    std::string resp = *pack::json::serialize(out, pack::Option::WithDefaults);
    logInfo("Return {}", resp);
}

Expected<void> Protocols::tryXmlPdc(const commands::protocols::In& in, uint16_t port) const
{
    impl::XmlPdc xml("http", in.address, port);
    if (auto prod = xml.get<impl::ProductInfo>("product.xml")) {
        if (!(prod->name == "Network Management Card" || prod->name == "HPE UPS Network Module")) {
            return unexpected("unsupported card type");
        }

        if (prod->protocol == "XML.V4") {
            return unexpected("unsupported XML.V4");
        }

        if (auto props = xml.get<impl::Properties>(prod->summary.summary.url)) {
            return {};
        } else {
            return unexpected(props.error());
        }
    } else {
        return unexpected(prod.error());
    }
}

Expected<void> Protocols::tryPowercom(const commands::protocols::In& in, uint16_t port) const
{
    neon::Neon ne("https", in.address, port);
    if (auto content = ne.get("etn/v1/comm/services/powerdistributions1")) {
        try {
            YAML::Node node = YAML::Load(*content);

            auto deviceType{node["device-type"].as<std::string>()};
            if (deviceType == "ups") {
                return {};
            }
            if (deviceType == "ats") {
                return {};
            }

            return unexpected("not supported device (" + deviceType + ")");
        } catch (const std::exception& e) {
            return unexpected("not supported device (" + std::string{e.what()} + ")");
        }
    } else {
        return unexpected(content.error());
    }
}

static Expected<void> timeoutConnect(int sock, const struct sockaddr* name, socklen_t namelen)
{
    pollfd    pfd;
    socklen_t optlen;
    int       optval;
    int       ret;

    if ((ret = connect(sock, name, namelen)) != 0 && errno == EINPROGRESS) {
        pfd.fd     = sock;
        pfd.events = POLLOUT;
        ret        = poll(&pfd, 1, 1000);
        if (ret == 1) {
            optlen = sizeof(optval);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &optval, &optlen) == 0) {
                errno = optval;
            }
        } else if (ret == 0) {
            errno = ETIMEDOUT;
        } else {
            return unexpected("poll failed");
        }
    }

    return {};
}

struct AutoRemove
{
    template <typename T, typename Func>
    AutoRemove(T& val, Func&& deleter)
        : m_deleter([=]() {
            deleter(val);
        })
    {
    }

    ~AutoRemove()
    {
        m_deleter();
    }
    std::function<void()> m_deleter;
};

Expected<void> Protocols::trySnmp(const commands::protocols::In& in, uint16_t port) const
{
    // fast check (no timeout, quite unreliable)
    // connect with a DGRAM socket, multiple tries to write in, check errors
    {
        addrinfo hints;
        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        addrinfo* addrInfo = nullptr;
        if (int ret = getaddrinfo(in.address.value().c_str(), std::to_string(port).c_str(), &hints, &addrInfo); ret != 0) {
            return unexpected(gai_strerror(ret));
        }
        AutoRemove addrInfoRem(addrInfo, &freeaddrinfo);

        int sock = socket(addrInfo->ai_family, addrInfo->ai_socktype | SOCK_NONBLOCK, addrInfo->ai_protocol);
        if (sock == -1) {
            return unexpected(strerror(errno));
        }
        AutoRemove sockRem(sock, &close);

        if (auto ret = timeoutConnect(sock, addrInfo->ai_addr, addrInfo->ai_addrlen); !ret) {
            return unexpected(strerror(errno));
        }

        // write in, check errors
        // we need a large number of tries to have a chance to get an error (experimentally 20)
        const int N = 20;
        for (int i = 0; i < N; i++) {
            auto r = write(sock, "X", 1);
            //logTrace("trySnmp {}:{} i: {}, r: {}", in.address, port, i, r);
            if (r == -1) {
                // here, we are sure that the device@port is not responsive
                return unexpected("Socket write failed");
            }
        }
        // here, we are not sure that the device@port is responsive
    }

    // reliable check (possible timeout) assuming SNMP v1/public
    // minimalistic SNMP v1 public introspection
    {
        auto session = fty::impl::Snmp::instance().session(in.address, port);
        if (!session) {
            logTrace("Create session failed");
            return unexpected("Create session failed");
        }
        if (auto ret = session->setCommunity("public"); !ret) {
            logTrace("session->setCommunitity failed ({})", ret.error());
            return unexpected(ret.error());
        }
        if (auto ret = session->setTimeout(500); !ret) { // msec
            logTrace("session->setTimeout failed ({})", ret.error());
            return unexpected(ret.error());
        }
        if (auto ret = session->open(); !ret) {
            logTrace("session->open() failed ({})", ret.error());
            return unexpected(ret.error());
        }
        const std::string sysOid{".1.3.6.1.2.1.1.2.0"};
        if (auto ret = session->read(sysOid); !ret) {
            logTrace("session->read() failed ({})", ret.error());
            return unexpected(ret.error());
        }
        // here, we are sure that the device@port is responsive
    }

    return {}; // ok
}

// =====================================================================================================================

} // namespace fty::disco::job
