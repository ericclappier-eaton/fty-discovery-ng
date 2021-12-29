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

// =====================================================================================================================

namespace address {

class AddressParser
{
public:
    AddressParser() = default;
    AddressParser(const AddressParser&);
    ~AddressParser() = default;

    fty::Expected<std::string> itIpInit(const std::string& startIP, const std::string& stopIP);
    fty::Expected<std::string> itIpNext();

    static bool isCidr(const std::string& cidr);
    static bool isRange(const std::string& range);

    static fty::Expected<std::pair<std::string, std::string>> cidrToLimits(const std::string& cidr);
    static fty::Expected<std::pair<std::string, std::string>> rangeToLimits(const std::string& range);

    static fty::Expected<std::vector<std::string>> getRangeIp(const std::string& range);
    static fty::Expected<std::vector<std::string>> getLocalRangeIp();

private:
    static fty::Expected<std::string> ntop(struct in_addr ip);
    static int maskNbBit(std::string mask);
    static fty::Expected<std::string> getAddrCidr(const std::string address, uint prefix);

    struct in_addr start;
	struct in_addr stop;
};

// =====================================================================================================================

} // namespace address
