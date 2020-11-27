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
#include <fty/split.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <set>
#include <unistd.h>

namespace fty::job {


// =====================================================================================================================

enum class Type
{
    Xml  = 1,
    Snmp = 2
};

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
    }
    return ss;
}

// =====================================================================================================================


void Protocols::run(const commands::protocols::In& in, commands::protocols::Out& out)
{
    if (in.address == "__fake__") {
        out.setValue({"nut_snmp", "nut_xml_pdc"});
        return;
    }

    if (!available(in.address)) {
        throw Error("Host is not available: {}", in.address.value());
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

    sortProtocols(protocols);

    for (const auto& prot : protocols) {
        switch (prot) {
            case Type::Snmp:
                out.append("nut_snmp");
                break;
            case Type::Xml:
                out.append("nut_xml_pdc");
                break;
        }
    }
    std::string resp = *pack::json::serialize(out);
    log_info("Return %s", resp.c_str());
}

Expected<void> Protocols::tryXmlPdc(const commands::protocols::In& in) const
{
    impl::XmlPdc xml(in.address);
    if (auto prod = xml.get<impl::ProductInfo>("product.xml")) {
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

static Expected<void> timeoutConnect(int sock, const struct sockaddr* name, socklen_t namelen)
{
    pollfd    pfd;
    socklen_t optlen;
    int       optval;
    int       ret;

    if ((ret = connect(sock, name, namelen)) != 0 && errno == EINPROGRESS) {
        pfd.fd     = sock;
        pfd.events = POLLOUT;
        if (poll(&pfd, 1, 1000) == 1) {
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
    std::string portStr = "161";

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

} // namespace fty::job
