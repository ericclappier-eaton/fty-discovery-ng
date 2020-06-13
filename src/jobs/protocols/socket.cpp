#include "socket.h"
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

namespace fty::protocol {

UdpSocket::UdpSocket(addrinfo* info)
    : m_sock(socket(info->ai_family, info->ai_socktype, info->ai_protocol))
    , m_info(info)
{
    int flags = fcntl(m_sock, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(m_sock, F_SETFL, flags);
    int sockopt = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_BROADCAST, &sockopt, sizeof(sockopt));
}

void UdpSocket::setTimeout(int timeout)
{
    assert(m_sock > 0);
    m_timeout = timeout;
}


Expected<int> UdpSocket::write(const std::string& data)
{
    assert(m_sock > 0);

    ssize_t retVal = sendto(m_sock, data.c_str(), data.size(), 0, m_info->ai_addr, m_info->ai_addrlen);
    if (retVal <= 0) {
        return unexpected() << strerror(errno);
    }
    return int(retVal);
}

Expected<std::string> UdpSocket::read()
{
    assert(m_sock > 0);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_sock, &fds);

    timeval timeout;
    timeout.tv_sec  = m_timeout / 1000;
    timeout.tv_usec = (m_timeout % 1000) * 1000;

    auto ret = select(m_sock + 1, &fds, nullptr, nullptr, &timeout);
    switch (ret) {
        case -1:
            return unexpected() << "Select error: " << strerror(errno);
        case 0:
            return unexpected() << "Timeout";
    }

    static constexpr size_t SIZE = 1024;
    std::string             buf;
    buf.resize(SIZE);
    (void)m_info;

    ssize_t          dataSize = 0;
    std::string      output;
    sockaddr_storage peerAddr;
    socklen_t        peerLen = m_info->ai_addrlen;

    while ((dataSize = recvfrom(m_sock, buf.data(), SIZE, 0, reinterpret_cast<sockaddr*>(&peerAddr), &peerLen)) > 0) {
        output += std::string(buf, 0, size_t(dataSize));
    }

    return output;
}

} // namespace fty::protocol
