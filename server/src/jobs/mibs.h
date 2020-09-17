/*  =========================================================================
    configure.h - Mibs discovery job

    Copyright (C) 2014 - 2020 Eaton

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
    =========================================================================
 */

#pragma once
#include "message.h"
#include <fty/expected.h>
#include <fty/thread-pool.h>

// =====================================================================================================================

namespace fty {
class MessageBus;
} // namespace fty

// =====================================================================================================================

namespace fty::job {

class Mibs : public Task<Mibs>
{
public:
    Mibs(const Message& in, MessageBus& bus);

    void operator()() override;

private:
    Message     m_in;
    MessageBus* m_bus;
};

} // namespace fty::job

// =====================================================================================================================
