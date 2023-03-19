#include "address.h"
#include <fty/expected.h>
#include <fty_common_base.h>
#include <fty_log.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <libcidr.h>
#ifdef __cplusplus
}
#endif

namespace address {

fty::Expected<std::string> AddressParser::itIpInit(const std::string& startIP, const std::string& stopIP) {

    if (startIP.empty()) {
        return fty::unexpected("start ip empty");
    }

    std::string stopIPCheck = stopIP;
    if (stopIPCheck.empty()) {
        stopIPCheck = startIP;
    }

    struct addrinfo hints;
    struct sockaddr_in *s_in = nullptr;
    struct addrinfo *res = nullptr;

    // Compute start IP
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(startIP.c_str(), nullptr, &hints, &res) == 0) {
        s_in = reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
        start = s_in->sin_addr;
        freeaddrinfo(res);
        res = nullptr;
        s_in = nullptr;
    }
    else {
        return fty::unexpected("Invalid start address : {}", startIP);
    }

    // Compute stop IP
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(stopIPCheck.c_str(), nullptr, &hints, &res) != 0) {
        start.s_addr = 0;
        return fty::unexpected("Invalid stop address : {}", stopIPCheck);
    }

    s_in = reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
    stop = s_in->sin_addr;
    freeaddrinfo(res);

    // Make sure start IP is less than stop IP
    if (ntohl(start.s_addr) > ntohl(stop.s_addr) ) {
        start.s_addr = 0;
        stop.s_addr = 0;
        return fty::unexpected("Error invalid address");
    }

    auto host = ntop(start);
    if (!host) {
        start.s_addr = 0;
        stop.s_addr = 0;
        return fty::unexpected("Error invalid address");
    }
    return *host;
}

fty::Expected<std::string> AddressParser::itIpNext() {

    if (start.s_addr == 0 && stop.s_addr == 0) {
        return fty::unexpected("Error invalid init");
    }

    // Check if this is the last address to scan
    if (start.s_addr == stop.s_addr) {
        return std::string("");
    }
    // Increment the address, need to pass address in host
    // byte order, then pass back in network byte order
    start.s_addr = htonl((ntohl(start.s_addr) + 1));

    auto res = ntop(start);
    if (!res) {
        return fty::unexpected("Error invalid address");
    }
    return *res;
}

bool AddressParser::isCidr(const std::string& cidr) {
    return (!cidr.empty() && cidr.find("/") != std::string::npos) ? true : false;
}

bool AddressParser::isRange(const std::string& range) {
    return (!range.empty() && range.find("-") != std::string::npos) ? true : false;
}

// TBD: remove minus subnet and broadcast address ???
fty::Expected<std::pair<std::string, std::string>> AddressParser::cidrToLimits(const std::string& cidr) {

    if (cidr.empty()) {
        return fty::unexpected("Error invalid address");;
    }

    int maskVal = 32;
    std::string firstIp;
    std::string mask;
    std::istringstream iss(cidr);
    if (std::getline(iss, firstIp, '/')) {
        if (std::getline(iss, mask, '/')) {
            maskVal = std::atoi(mask.c_str());
        }
    }

    if (maskVal > 32) {
        return fty::unexpected("Error invalid mask");
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    AddressParser ip;
    struct addrinfo *res = nullptr;
    struct sockaddr_in *sIn = nullptr;
    if (getaddrinfo(firstIp.c_str(), nullptr, &hints, &res) == 0) {
        sIn = reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
        ip.start = sIn->sin_addr;
        freeaddrinfo(res);
    }

    uint32_t maskBit{0};
    if (maskVal > 0) {
        maskVal --;
        maskBit = 0x80000000;
        maskBit >>= maskVal;
        maskBit --;
    }
    else {
        maskBit = 0xffffffff;
    }

    ip.stop.s_addr = htonl(ntohl(ip.start.s_addr) | maskBit);
    ip.start.s_addr = htonl(ntohl(ip.start.s_addr) & (~maskBit));

    auto startIp = ntop(ip.start);
    if (!startIp) {
        return fty::unexpected("Error invalid start address");
    }

    auto stopIp = ntop(ip.stop);
    if (!stopIp) {
        return fty::unexpected("Error invalid stop address");
    }
    logTrace("cidrToLimits: {}-{}", *startIp, *stopIp);
    return std::make_pair(*startIp, *stopIp);
}

fty::Expected<std::pair<std::string, std::string>> AddressParser::rangeToLimits(const std::string& range) {

    if (range.empty()) {
        return fty::unexpected("Error invalid range");
    }

    std::string startIp;
    std::string stopIp;
    std::istringstream iss(range);
    if (std::getline(iss, startIp, '-')) {
        if (!std::getline(iss, stopIp, '-')) {
            return fty::unexpected("Error invalid range");
        }
    }
    else {
        return fty::unexpected("Error invalid range");
    }

    if (startIp.empty() || stopIp.empty()) {
        return fty::unexpected("Error invalid range");
    }
    logTrace("rangeToLimits: {}-{}", startIp, stopIp);
    return std::make_pair(startIp, stopIp);
}

fty::Expected<std::vector<std::string>> AddressParser::getRangeIp(const std::string& range) {

    std::pair<std::string, std::string> addrLimits;
    if (AddressParser::isRange(range)) {
        if (auto res = AddressParser::rangeToLimits(range); !res) {
            return fty::unexpected("Error bad range");
        }
        else {
            addrLimits = *res;
        }
    }
    else if (AddressParser::isCidr(range)) {
        if (auto res = AddressParser::cidrToLimits(range); !res) {
            return fty::unexpected("Error bad range");
        }
        else {
            addrLimits = *res;
        }
    }
    else {
        return fty::unexpected("bad range");
    }

    int count = 0;
    std::vector<std::string> listIp;
    AddressParser address;
    auto addr = address.itIpInit(addrLimits.first, addrLimits.second);
    if (addr) {
        count ++;
        listIp.push_back(*addr);
        while (1) {
            auto addr_next = address.itIpNext();
            if (!addr_next) {
                return fty::unexpected("list ip next error");
            }
            else if ((*addr_next).empty()) {
                break;
            }
            count ++;
            listIp.push_back(*addr_next);
        }
    }
    else {
        return fty::unexpected("list ip init error");
    }
    logTrace("getRangeIp: add {} address(es)", count);
    return listIp;
}

fty::Expected<std::vector<std::string>> AddressParser::getLocalRangeIp() {

    std::vector<std::string> fullListIp;
    int             s, family, prefix;
    char            host[NI_MAXHOST];
    struct ifaddrs *ifaddr = nullptr, *ifa = nullptr;
    std::string     addr, netm, addrmask;

    int scanSize = 0;
    if (getifaddrs(&ifaddr) != -1) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (streq(ifa->ifa_name, "lo"))
                continue;
            if (ifa->ifa_addr == NULL)
                continue;

            family = ifa->ifa_addr->sa_family;
            if (family != AF_INET)
                continue;

            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                logDebug("IP address parsing error for {} : {}", ifa->ifa_name, gai_strerror(s));
                continue;
            } else
                addr.assign(host);

            if (ifa->ifa_netmask == NULL) {
                logDebug("No netmask found for {}", ifa->ifa_name);
                continue;
            }

            family = ifa->ifa_netmask->sa_family;
            if (family != AF_INET)
                continue;

            s = getnameinfo(ifa->ifa_netmask, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                logDebug("Netmask parsing error for {} : {}", ifa->ifa_name, gai_strerror(s));
                continue;
            } else
                netm.assign(host);

            prefix = 0;
            addrmask.clear();

            prefix = maskNbBit(netm);

            // all the subnet (1 << (32- prefix) ) minus subnet and broadcast address
            if (prefix <= 30) {
                scanSize += ((1 << (32 - prefix)) - 2);
            }
            else {
                // 31/32 prefix special management
                scanSize += (1 << (32 - prefix));
            }

            logTrace("getLocalRangeIp addr={}, prefix={}", addr, prefix);
            auto res = AddressParser::getAddrCidr(addr, fty::convert<unsigned int>(prefix));
            if (res) {
                addrmask.clear();
                addrmask.assign(*res);
                logInfo("getLocalRangeIp subnet found for {} : {}", ifa->ifa_name, addrmask);
                auto listIp = AddressParser::getRangeIp(addrmask);
                if (listIp) {
                    fullListIp.insert(fullListIp.end(), listIp->begin(), listIp->end());
                }
                else {
                    logDebug("getLocalRangeIp error: {}", listIp.error());
                }
            }
            else {
                logDebug("getLocalRangeIp error: {}", res.error());
            }
        }
        freeifaddrs(ifaddr);
    }
    logTrace("getLocalRangeIp: scan_size={}, fullListIp={}", scanSize, fullListIp.size());
    return fullListIp;
}

fty::Expected<std::string> AddressParser::getAddrCidr(const std::string& address, uint prefix) {

    auto isValid = [](CIDR* cidr) {
        if (cidr_get_proto(cidr) == CIDR_IPV4) {
            in_addr  inAddr;
            memset(&inAddr, 0, sizeof(inAddr));
            if (cidr_to_inaddr(cidr, &inAddr)) {
                return (inAddr.s_addr != 0); // 0.0.0.0 address
            } else {
                return false;
            }
        }
        return false;
    };

    std::string text = address + "/" + std::to_string(prefix);
    CIDR* cidr = cidr_from_str(text.c_str());
    if (!isValid(cidr)) {
        cidr_free(cidr);
        return fty::unexpected("Bad address");
    }

    CIDR *newCidr = cidr_addr_network(cidr);
    cidr_free(cidr);

    std::string addr = "";
    char *cstr = cidr_to_str(newCidr, CIDR_NOFLAGS);
    if (cstr) {
        addr = cstr;
        free(cstr);
    }
    cidr_free(newCidr);
    return addr;
}

fty::Expected<std::string> AddressParser::ntop(struct in_addr ip) {

    char hbuf[NI_MAXHOST];
    struct sockaddr_in in;
    memset(&in, 0, sizeof(in));
    in.sin_addr = ip;
    in.sin_family = AF_INET;
    if (getnameinfo(reinterpret_cast<const sockaddr *>(&in),
                    sizeof(struct sockaddr_in),
                    hbuf, sizeof(hbuf), nullptr, 0, NI_NUMERICHOST) != 0) {
        return fty::unexpected("Host not valid");
    }
    return std::string(hbuf);
}

int AddressParser::maskNbBit(const std::string& mask)
{
    size_t      pos;
    int         res = 0;
    std::string part;
    std::string maskComputed { mask };
    while ((pos = maskComputed.find(".")) != std::string::npos) {
        part = maskComputed.substr(0, pos);
        if (part == "255")
            res += 8;
        else if (part == "254")
            return res + 7;
        else if (part == "252")
            return res + 6;
        else if (part == "248")
            return res + 5;
        else if (part == "240")
            return res + 4;
        else if (part == "224")
            return res + 3;
        else if (part == "192")
            return res + 2;
        else if (part == "128")
            return res + 1;
        else if (part == "0")
            return res;
        else // error
            return -1;

        maskComputed.erase(0, pos + 1);
    }

    if (maskComputed == "255")
        res += 8;
    else if (maskComputed == "254")
        res += 7;
    else if (maskComputed == "252")
        res += 6;
    else if (maskComputed == "248")
        res += 5;
    else if (maskComputed == "240")
        res += 4;
    else if (maskComputed == "224")
        res += 3;
    else if (maskComputed == "192")
        res += 2;
    else if (maskComputed == "128")
        res += 1;
    else if (maskComputed == "0")
        return res;
    else
        return -1;

    return res;
}

}
