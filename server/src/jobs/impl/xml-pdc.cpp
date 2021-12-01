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

#include "xml-pdc.h"

namespace fty::disco::impl {

// =====================================================================================================================

std::optional<std::string> Properties::value(const std::string& key)
{
    auto item = values.find([&](const Value& val) {
        return val.key == key;
    });

    if (item) {
        return item->value;
    }
    return std::nullopt;
}

// =====================================================================================================================

XmlPdc::XmlPdc(const std::string& scheme, const std::string& address, uint16_t port)
    : m_ne(scheme, address, port)
{
}

// =====================================================================================================================

} // namespace fty::disco::impl
