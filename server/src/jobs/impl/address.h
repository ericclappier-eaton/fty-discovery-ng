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

#pragma once

#include <fty/expected.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <vector>

// =====================================================================================================================

namespace address {

class AddressParser
{
public:
    AddressParser() {
        memset(&start, 0, sizeof(start));
        memset(&stop, 0, sizeof(stop));
    };
    AddressParser(const AddressParser&);
    ~AddressParser() = default;

    // return the first ip from iteration
    fty::Expected<std::string> itIpInit(const std::string& startIP, const std::string& stopIP);
    // return the next IP from iteration
    fty::Expected<std::string> itIpNext();

    // test if it is a cidr address
    static bool isCidr(const std::string& cidr);
    // test if it is a range address
    static bool isRange(const std::string& range);

    // get string start and stop addresses from cdir
    static fty::Expected<std::pair<std::string, std::string>> cidrToLimits(const std::string& cidr);
    // get string start and stop addresses from range
    static fty::Expected<std::pair<std::string, std::string>> rangeToLimits(const std::string& range);

    // get range ip list
    static fty::Expected<std::vector<std::string>> getRangeIp(const std::string& range);
    // get local range ip list
    static fty::Expected<std::vector<std::string>> getLocalRangeIp();

private:
    // get address from cidr
    static fty::Expected<std::string> getAddrCidr(const std::string& address, uint prefix);
    // return string ip address from address struct
    static fty::Expected<std::string> ntop(struct in_addr ip);
    // return mask computed
    static int maskNbBit(const std::string& mask);

    struct in_addr start;
    struct in_addr stop;
};

// =====================================================================================================================

} // namespace address
