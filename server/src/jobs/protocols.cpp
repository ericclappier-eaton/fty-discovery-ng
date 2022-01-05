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
#include <fty/string-utils.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <set>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

namespace fty::disco::job {


// =====================================================================================================================

inline std::ostream& operator<<(std::ostream& ss, Type type)
{
    switch (type) {
        case Type::Snmp:
            ss << "Snmp";
            break;
        case Type::Xml:
            ss << "Xml";
            break;
        case Type::Powercom:
            ss << "GenApi";
            break;
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

const std::pair<std::string, std::string> Protocols::splitPortFromProtocol(const std::string &str) {
    // split protocol and port with format "<protocol>:<port>"
    std::string protocol, port;
    auto pos = str.find(":");
    // If port found,
    if (pos != std::string::npos) {
        // get first protocol
        protocol = std::move(str.substr(0, pos));
        // and get second port number
        port = std::move(str.substr(pos + 1));
    }
    else {
        // No port found, return only protocol
        protocol = str;
    }
    return make_pair(protocol, port);
}

std::optional<uint16_t> Protocols::getPort(const std::string& protocolIn, const commands::protocols::In& in) {
    if (in.port != 0) {
        logDebug("getPort: get port={}", in.port.value());
        return in.port;
    }
    else if (in.protocols.size() != 0) {
        for (const auto& protocolRequested : in.protocols) {
            auto split = Protocols::splitPortFromProtocol(protocolRequested);
            std::string protocol = split.first;
            if (protocol == protocolIn) {
                if (!split.second.empty()) {
                    logDebug("getPort: get port={} from {} protocol", split.second, protocolIn);
                    return std::atoi(split.second.c_str());
                }
            }
        }
    }
    return std::nullopt;
}

// =====================================================================================================================
Expected<std::vector<Type>> Protocols::getProtocols(const commands::protocols::In& in) const
{
    if (!available(in.address)) {
        return unexpected("Host is not available: {}", in.address.value());
    }

    std::vector<Type> protocols;

    if (auto res = tryXmlPdc(in)) {
        protocols.emplace_back(Type::Xml);
        log_info("Found XML device");
    } else {
        log_info("Skipped xml_pdc, reason: %s", res.error().c_str());
    }

    if (auto res = trySnmp(in)) {
        protocols.emplace_back(Type::Snmp);
        log_info("Found SNMP device");
    } else {
        log_info("Skipped snmp, reason: %s", res.error().c_str());
    }

    if (auto res = tryPowercom(in)) {
        protocols.emplace_back(Type::Powercom);
        log_info("Found Powercon device");
    } else {
        log_info("Skipped GenApi, reason: %s", res.error().c_str());
    }

    sortProtocols(protocols);
    return protocols;
}

void Protocols::run(const commands::protocols::In& in, commands::protocols::Out& out)
{
    auto protocols = getProtocols(in);
    if (!protocols) {
        throw Error(protocols.error().c_str());
    }

    for (const auto& prot : *protocols) {
        out.append(getProtocolStr(prot));
    }
    std::string resp = *pack::json::serialize(out);
    log_info("Return %s", resp.c_str());
}

Expected<void> Protocols::tryXmlPdc(const commands::protocols::In& in) const
{
    auto port = Protocols::getPort("nut_xml_pdc", in);
    impl::XmlPdc xml(in.address , (port != std::nullopt) ? *port : 80);
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

Expected<void> Protocols::tryPowercom(const commands::protocols::In& in) const
{
    auto port = getPort("nut_powercom", in);
    neon::Neon ne(in.address, (port != std::nullopt) ? *port : 80);
    if (auto content = ne.get("etn/v1/comm/services/powerdistributions1")) {
        try {
            YAML::Node node = YAML::Load(*content);

            if (node["device-type"].as<std::string>() == "ups") {
                return {};
            }

            return unexpected("not supported device");
        } catch (const std::exception&) {
            return unexpected("not supported device");
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

Expected<void> Protocols::trySnmp(const commands::protocols::In& in) const
{
    auto port = getPort("nut_snmp", in);
    std::string portStr = (port != std::nullopt) ? std::to_string(*port) : "161";
    logDebug("getPort: portStr={}", portStr);

    addrinfo hints;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    addrinfo* addrInfo;
    if (int ret = getaddrinfo(in.address.value().c_str(), portStr.c_str(), &hints, &addrInfo); ret != 0) {
        return unexpected(gai_strerror(ret));
    }
    AutoRemove addInfoRem(addrInfo, &freeaddrinfo);

    int sock = 0;

    if ((sock = socket(addrInfo->ai_family, addrInfo->ai_socktype | SOCK_NONBLOCK, addrInfo->ai_protocol)) == -1) {
        return unexpected(strerror(errno));
    }
    AutoRemove sockRem(sock, &close);

    if (timeoutConnect(sock, addrInfo->ai_addr, addrInfo->ai_addrlen) == 0) {
        return unexpected(strerror(errno));
    }

    int ret = 1;
    for (int i = 0; i <= 3; i++) {
        if (write(sock, "X", 1) == 1) {
            ret = 1;
        } else {
            ret = -1;
        }
    }

    if (ret == -1) {
        return unexpected("cannot write");
    }

    return {};
}

void Protocols::sortProtocols(std::vector<Type>& protocols)
{
    // Just prefer XML_PDC
    std::sort(protocols.begin(), protocols.end());
}


// =====================================================================================================================

} // namespace fty::disco::job
