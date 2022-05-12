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
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <unistd.h>

// =====================================================================================================================

static std::mutex g_mutexAvailable;

inline bool available(const std::string& address_)
{
    static const std::string httpPrefix = "http://";

    std::lock_guard<std::mutex> lock(g_mutexAvailable);

    std::string address{address_};
    if (address.find(httpPrefix) == 0) {
        address = address.substr(httpPrefix.size());
    }

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = 0;
    hints.ai_protocol = 0;

    addrinfo* result{nullptr};
    if (getaddrinfo(address.c_str(), nullptr, &hints, &result) != 0) {
        return false;
    }

    bool available = false;
    for (addrinfo* rp = result; rp != nullptr && !available; rp = rp->ai_next) {
        int sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock != -1) {
            if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1) {
                available = true;
            }
            close(sock);
        }
    }

    freeaddrinfo(result);

    return available;
}

// =====================================================================================================================
