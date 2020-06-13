#pragma once
#include <fty/expected.h>

struct addrinfo;

namespace fty::protocol {

class UdpSocket
{
public:
    UdpSocket(addrinfo* info);

    void setTimeout(int timeout);

    Expected<int>         write(const std::string& data);
    Expected<std::string> read();

private:
    int       m_sock;
    addrinfo* m_info;
    int       m_timeout = 1000; // 1 sec
};

} // namespace fty::protocol
