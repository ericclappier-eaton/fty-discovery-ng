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
#include <fty_log.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <set>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

namespace fty::disco::job {

// =====================================================================================================================
std::optional<const ConfigDiscovery::Protocol>
Protocols::findProtocol(const config_protocol_t& config_protocol, const commands::protocols::In& in) {
    // check if input protocols list is NOT empty
    if (in.protocols.size() != 0) {
        for (const auto& protocol : in.protocols) {

            // test if option found in options
            if (protocol.protocol == config_protocol.protocol) {
                // return option found in options
                return protocol;
            }
        }
    }
    // input protocols list is empty, return default configuration
    // NOTE: protocol to search must be NOT unknown
    else if (config_protocol.protocol != ConfigDiscovery::Protocol::Type::Unknown) {
        ConfigDiscovery::Protocol res;
        res.protocol = config_protocol.protocol;
        res.ports.append(config_protocol.defaultPort);
        return res;
    }

    // no option available, any protocol filtered
    return std::nullopt;
}

// =====================================================================================================================
Expected<commands::protocols::Out> Protocols::getProtocols(const commands::protocols::In& in) const
{
    using namespace commands::protocols;

    if (!available(in.address)) {
        return unexpected("Host is not available: {}", in.address.value());
    }

    commands::protocols::Out out;
    // supported protocols, tokenized with default port
    // in *order* of preferences
    config_protocol_t tries[] = {
        {ConfigDiscovery::Protocol::Type::Powercom, 443},
        {ConfigDiscovery::Protocol::Type::XmlPdc,    80},
        {ConfigDiscovery::Protocol::Type::Snmp,     161}
    };

    // for each protocol
    for (auto& aux : tries) {
        auto& protocol = out.append();
        std::stringstream ss;
        ss << aux.protocol;
        protocol.protocol  = ss.str();
        protocol.port      = aux.defaultPort;
        protocol.reachable = false; // default, port is not reachable
        protocol.available = Return::Available::No; // and protocol is not available
        //protocol.ignored   = true;  // default, filtered

        // check if current protocol to search is in the input protocols list.
        // NOTE: if input protocols list is empty, return current default configuration. In
        // this case, all protocols will be scan with default port.
        auto find = Protocols::findProtocol(aux, in);
        if (find /*&& !find->ignore*/) {
            // TBD: To be reworked with scan auto interface (possibility to add more then one port)
            // test protocol for each port
            // Note: for a same protocol, stop for first port which responding
            //for (int iPort = 0; iPort < find->ports.size() && !protocol.reachable; iPort ++) {

                // TBD: Take first port available (workaround)
                if (find->ports.size() > 0 && find->ports[0] != 0) {
                    protocol.port = find->ports[0];
                }

                //if (find->ports[iPort] != 0) protocol.port = find->ports[iPort];
                //protocol.ignored = false;

                // try to reach server
                switch (aux.protocol) {
                    case ConfigDiscovery::Protocol::Type::Powercom: {
                        if (auto res = tryPowercom(in.address.value(), static_cast<uint16_t>(protocol.port))) {
                            logInfo("Found Powercom device on port {}", protocol.port);
                            protocol.reachable = true; // port is reachable
                            protocol.available = Return::Available::Yes; // port is available
                        }
                        else {
                            logInfo("Skipped GenApi/{}, reason: {}", protocol.port.value(), res.error());
                        }
                        break;
                    }
                    case ConfigDiscovery::Protocol::Type::XmlPdc: {
                        if (auto res = tryXmlPdc(in.address.value(), static_cast<uint16_t>(protocol.port))) {
                            logInfo("Found XML device on port {}", protocol.port);
                            protocol.reachable = true; // port is reachable
                            protocol.available = Return::Available::Yes; // port is available
                        }
                        else {
                            logInfo("Skipped xml_pdc/{}, reason: {}", protocol.port.value(), res.error());
                        }
                        break;
                    }
                    case ConfigDiscovery::Protocol::Type::Snmp: {
                        if (auto res = trySnmp(in.address.value(), static_cast<uint16_t>(protocol.port))) {
                            logInfo("Found SNMP device on port {}", protocol.port);
                            protocol.reachable = true; // port is reachable
                            protocol.available = Return::Available::Maybe; // port is maybe available
                        }
                        else {
                            logInfo("Skipped SNMP/{}, reason: {}", protocol.port.value(), res.error());
                        }
                        break;
                    }
                    default:
                        logError("protocol not handled (type: {})", aux.protocol);
                }
            }
        //}
    }
    return out;
}

void Protocols::run(const commands::protocols::In& in, commands::protocols::Out& out)
{
    logDebug("Protocols::run started");

    auto protocols = getProtocols(in);
    if (!protocols) {
        throw Error(protocols.error().c_str());
    }
    out = *protocols;
    if (auto resp = pack::json::serialize(out, pack::Option::WithDefaults)) {
        logInfo("Return {}", *resp);
    }
    else {
        logError("Error during serialisation: {}", resp.error());
    }
}

Expected<void> Protocols::tryXmlPdc(const std::string& address, uint16_t port) const
{
    logDebug("Protocols::tryXmlPdc {}:{}", address, port);

    impl::XmlPdc xml("http", address, port);
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

Expected<void> Protocols::tryPowercom(const std::string& address, uint16_t port) const
{
    logDebug("Protocols::tryPowercom {}:{}", address, port);

    neon::Neon ne("https", address, port);
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

Expected<void> Protocols::trySnmp(const std::string& address, uint16_t port) const
{
    logDebug("Protocols::trySnmp {}:{}", address, port);

    // fast check (no timeout, quite unreliable)
    // connect with a DGRAM socket, multiple tries to write in, check errors
    {
        addrinfo hints;
        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        addrinfo* addrInfo = nullptr;
        if (int ret = getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &addrInfo); ret != 0) {
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
            //logTrace("trySnmp {}:{} i: {}, r: {}", address, port, i, r);
            if (r == -1) {
                // here, we are sure that the device@port is not responsive
                return unexpected("Socket write failed");
            }
        }
        // here, we are not sure that the device@port is responsive
    }
    return {}; // ok
}

// =====================================================================================================================

} // namespace fty::disco::job
