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
#include "neon.h"
#include <fty/expected.h>
#include <pack/pack.h>

namespace fty::disco::impl {

// =====================================================================================================================

/// Properties page structure
class Properties : public pack::Node
{
public:
    struct Value : public pack::Node
    {
        pack::String key   = FIELD("a::name");
        pack::String value = FIELD("cdata");

        using pack::Node::Node;
        META(Value, key, value);
    };

public:
    pack::String            auth   = FIELD("a::authentication");
    pack::ObjectList<Value> values = FIELD("OBJECT");

public:
    using pack::Node::Node;
    META(Properties, auth, values);

public:
    std::optional<std::string> value(const std::string& key);
};

// =====================================================================================================================

/// Product info page structure
class ProductInfo : public pack::Node
{
public:
    struct Page : public pack::Node
    {
        pack::String url      = FIELD("a::url");
        pack::String security = FIELD("a::security");
        pack::String mode     = FIELD("a::mode");

        using pack::Node::Node;
        META(Page, url, security, mode);
    };

    struct Summary : public pack::Node
    {
        Page summary = FIELD("XML_SUMMARY_PAGE");
        Page config  = FIELD("CENTRAL_CFG");
        Page logs    = FIELD("CSV_LOGS");

        using pack::Node::Node;
        META(Summary, summary, config, logs);
    };

public:
    pack::String name     = FIELD("a::name");
    pack::String type     = FIELD("a::type");
    pack::String version  = FIELD("a::version");
    pack::String protocol = FIELD("a::protocol");
    Summary      summary  = FIELD("SUMMARY");

public:
    using pack::Node::Node;
    META(ProductInfo, name, type, version, protocol, summary);
};

// =====================================================================================================================

/// Reads XML_PDC basic information from endpoint
class XmlPdc
{
public:
    XmlPdc(const std::string& scheme, const std::string& address, uint16_t port);

    template <typename T>
    Expected<T> get(const std::string& uri) const
    {
        auto content = m_ne.get(uri);
        if (!content) {
            return unexpected(content.error());
        }
        T info;
        neon::deserialize(*content, info);
        return std::move(info);
    }

private:
    neon::Neon m_ne;
};

// =====================================================================================================================

} // namespace fty::disco::impl
