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

#include "assets.h"
#include "impl/nut.h"
#include "impl/ping.h"

namespace fty::job {

void Assets::run(const commands::assets::In& in, commands::assets::Out& out)
{
    if (!available(in.address)) {
        throw Error("Host is not available: {}", in.address.value());
    }

    if (auto res = protocol::properties(in)) {
        std::string str = *pack::json::serialize(*res);
        log_debug("output %s", str.c_str());

        out = *res;
    } else {
        throw Error(res.error());
    }
}

} // namespace fty::job
