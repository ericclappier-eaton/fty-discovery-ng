#include "discover.h"
#include "protocols/snmp.h"
#include "protocols/xml.h"
#include "src/message-bus.h"
#include <fty/fty-log.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

namespace fty::job {

namespace {
    struct Response : public pack::Node
    {
        using pack::Node::Node;

        pack::StringList            protocols = FIELD("protocols");
        pack::String                error     = FIELD("error");
        pack::Enum<Message::Status> status    = FIELD("status");

        META_FIELDS(result, error)
    };
} // namespace

Discover::Discover(const Message& in, MessageBus& bus)
    : m_in(in)
    , m_bus(&bus)
{
}

void Discover::operator()()
{
    Response result;
    Message  reply;
    reply.meta.to   = m_in.meta.from;
    reply.meta.from = m_in.meta.to;

    auto sendReply = [&]() {
        reply.meta.status = result.status;
        if (result.status == Message::Status::ok) {
            reply.userData.append(*pack::json::serialize(result.protocols));
        } else {
            reply.userData.append(*pack::json::serialize(result.error));
        }
        m_bus->reply("discover", m_in, reply);
    };

    if (m_in.userData.empty()) {
        result.status = Message::Status::ko;
        result.error  = "Wrong input data";
        sendReply();
        return;
    }

    std::string ipAddress = m_in.userData[0];

    protocol::Snmp snmp;
    if (auto res = snmp.discover(ipAddress)) {
        if (*res) {
            result.protocols.append("NUT_SNMP");
        }
    } else {
        logError() << res.error();
    }

    protocol::Xml xml;
    if (auto res = xml.discover(ipAddress)) {
        if (*res) {
            result.protocols.append("NUT_XML_PDC");
        }
    } else {
        logError() << res.error();
    }

    result.status = Message::Status::ok;
    sendReply();
}

Expected<bool> Discover::portIsOpen(const std::string& address, uint16_t port)
{
    std::string portStr = std::to_string(port);
    addrinfo    hints;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    addrinfo* result;
    if (int ret = getaddrinfo(address.c_str(), portStr.c_str(), &hints, &result); ret != 0) {
        return unexpected(gai_strerror(ret));
    }

    int sock = -1;

    for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) {
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 100 * 1000; // 100 ms to connect
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
            logInfo() << "connected " << sock;

            if (getnameinfo(rp->ai_addr, rp->ai_addrlen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                    NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
                logInfo() << "host " << hbuf << ", port " << sbuf;
            }
            break;
        }


        close(sock);
        sock = -1;
    }

    freeaddrinfo(result);

    if (sock != -1) {
        close(sock);
        return true;
    }

    return false;
}

} // namespace fty::job
