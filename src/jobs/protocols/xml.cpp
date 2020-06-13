#include "xml.h"
#include "neon.h"
#include "socket.h"
#include <fty/fty-log.h>
#include <netdb.h>
#include <pack/pack.h>
#include <string.h>


namespace fty::protocol {

struct ProductInfo : public pack::Node
{
    struct Page : public pack::Node
    {
        using pack::Node::Node;

        pack::String url      = FIELD("a::url");
        pack::String security = FIELD("a::security");
        pack::String mode     = FIELD("a::mode");

        META_FIELDS(Page, url, security, mode)
    };

    struct Summary : public pack::Node
    {
        using pack::Node::Node;

        Page summary = FIELD("XML_SUMMARY_PAGE");
        Page config  = FIELD("CENTRAL_CFG");
        Page logs    = FIELD("CSV_LOGS");

        META_FIELDS(Summary, summary, config, logs)
    };

    using pack::Node::Node;

    pack::String name    = FIELD("a::name");
    pack::String type    = FIELD("a::type");
    pack::String version = FIELD("a::version");
    Summary      summary = FIELD("SUMMARY");

    META_FIELDS(ProductInfo, name, type, version, summary)
};

template <typename Func>
void forEachInterface(const std::string& address, uint16_t udpPort, Func&& fnc)
{
    std::string portStr = std::to_string(udpPort);
    addrinfo    hints;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;
    hints.ai_protocol = 0;

    addrinfo* result;
    if (int ret = getaddrinfo(address.c_str(), portStr.c_str(), &hints, &result); ret != 0) {
        logError() << gai_strerror(ret);
    }

    for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

        if (getnameinfo(rp->ai_addr, rp->ai_addrlen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                NI_NUMERICHOST | NI_NUMERICSERV | NI_DGRAM) == 0) {
            logInfo() << "host '" << hbuf << "', port " << sbuf;
        }

        if (fnc(rp)) {
            break;
        }
    }

    freeaddrinfo(result);
}

Xml::Xml()
{
}

Expected<bool> Xml::discover(const std::string& addr) const
{
    /*forEachInterface(addr, 4679, [](addrinfo* info) {
        UdpSocket soc(info);
        soc.setTimeout(500);

        if (auto ret = soc.write("<SCAN_REQUEST/>"); !ret) {
            logError() << "Discovery xml/pdc udp write error: " << ret.error();
            return false;
        }

        if (auto ret = soc.read(); !ret) {
            logError() << "Discovery xml/pdc udp read error: " << ret.error();
            return false;
        }
        return true;
    });*/

    neon::Neon ne;
    if (ne.connect(addr, 80)) {
        if (auto cnt = ne.get("product.xml")) {
            logInfo() << *cnt;
            ProductInfo info;
            neon::deserialize(*cnt, info);
            logInfo() << info.dump();
            logInfo() << "name: " << info.name;
            return true;
        } else {
            logError() << cnt.error();
            return false;
        }
    }
    return false;
}

} // namespace fty::protocol
