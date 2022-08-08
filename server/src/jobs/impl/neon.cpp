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

#include "neon.h"
#include <fty/string-utils.h>
#include <fty_log.h>
#include <iostream>
#include <neon/ne_request.h>
#include <neon/ne_session.h>
#include <neon/ne_xml.h>
#include <pack/visitor.h>

static int verify_fn(void* /*userdata*/, int /*failures*/, const ne_ssl_certificate* /*cert*/)
{
    return 0; // trust cert
}

namespace neon {

Neon::Neon(const std::string& scheme, const std::string& address, uint16_t port, uint16_t timeout)
{
    ne_sock_init();
    //ne_debug_init(stderr, NE_DBG_HTTP | NE_DBG_HTTPBODY);

    m_session = ne_session_create(scheme.c_str(), address.c_str(), port);

    if (!m_session) {
        logError("ne_session_create() failed");
        return;
    }

    // trust any certificate
    ne_ssl_set_verify(m_session, verify_fn, nullptr);

    ne_set_connect_timeout(m_session, timeout);
    ne_set_read_timeout(m_session, timeout);
}

Neon::~Neon()
{
    if (m_session) {
        ne_session_destroy(m_session);
        m_session = nullptr;
    }
    ne_sock_exit();
}

fty::Expected<std::string> Neon::get(const std::string& path) const
{
    if (!m_session) {
        return fty::unexpected("m_session not initialized");
    }

    std::string rpath = "/" + path;
    ne_request* request = ne_request_create(m_session, "GET", rpath.c_str());
    if (!request) {
        return fty::unexpected("ne_request_create() failed");
    }

    std::string body;

    do {
        int  stat   = ne_begin_request(request);
        auto status = ne_get_status(request);
        if (stat != NE_OK) {
            ne_request_destroy(request);
            if (!status->code) {
                return fty::unexpected(ne_get_error(m_session));
            }
            return fty::unexpected("non-NE_OK, status: {} {}", status->code, status->reason_phrase);
        }

        if (status->code != 200) {
            ne_request_destroy(request);
            return fty::unexpected("NE_OK, status: {} {}", status->code, status->reason_phrase);
        }

        body.clear();
        std::array<char, 1024> buffer;

        ssize_t bytes = 0;
        while ((bytes = ne_read_response_block(request, buffer.data(), buffer.size())) > 0) {
            body += std::string(buffer.data(), size_t(bytes));
        }
    } while (ne_end_request(request) == NE_RETRY);

    ne_request_destroy(request);

    return fty::Expected<std::string>(body);
}

// =====================================================================================================================

struct NeonNode
{
    std::string           name;
    std::string           value;
    std::vector<NeonNode> children;
    NeonNode*             parent = nullptr;

    void dump(std::ostream& st, size_t level = 0) const
    {
        std::string indent(level * 4, ' ');
        if (parent == nullptr) {
            st << indent << "[root]\n";
        } else {
            st << indent << "[" << name << "]";
            if (!value.empty()) {
                st << " = " << value;
            }
            st << "\n";
        }

        for (const auto& child : children) {
            child.dump(st, level + 1);
        }
    }
};

// =====================================================================================================================

template <pack::Type ValType>
struct Convert
{
    using CppType = typename pack::ResolveType<ValType>::type;

    static void decode(pack::Value<ValType>& node, const NeonNode& neon)
    {
        node = fty::convert<CppType>(neon.value);
    }

    static void decode(pack::ValueList<ValType>& /*node*/, const NeonNode& /*neon*/)
    {
    }

    static void decode(pack::ValueMap<ValType>& /*node*/, const NeonNode& /*neon*/)
    {
    }
};

// =====================================================================================================================

class NeonDeserializer : public pack::Deserialize<NeonDeserializer>
{
public:
    template <typename T>
    static void unpackValue(T& val, const NeonNode& neon)
    {
        Convert<T::ThisType>::decode(val, neon);
    }

    static void unpackValue(pack::INode& node, const NeonNode& neon)
    {
        for (auto& it : node.fields()) {
            auto found = std::find_if(neon.children.begin(), neon.children.end(), [&](const NeonNode& child) {
                return child.name == it->key();
            });
            if (found != neon.children.end()) {
                visit(*it, *found);
            }
        }
    }

    static void unpackValue(pack::IEnum& /*en*/, const NeonNode& /*neon*/)
    {
    }

    static void unpackValue(pack::IObjectMap& /*list*/, const NeonNode& /*neon*/)
    {
    }

    static void unpackValue(pack::IObjectList& list, const NeonNode& neon)
    {
        NeonNode* parent = neon.parent;
        if (!parent) {
            return;
        }
        for (const auto& ch : parent->children) {
            if (ch.name != neon.name) {
                continue;
            }

            auto& item = list.create();
            visit(item, ch);
        }
    }

    static void unpackValue(pack::IProtoMap& /*map*/, const NeonNode& /*neon*/)
    {
    }

    static void unpackValue(pack::IVariant& /*var*/, const NeonNode& /*neon*/)
    {
    }
};

// =====================================================================================================================

class Parser
{
public:
    Parser(NeonNode& root)
        : m_current(&root)
    {
    }

    void parse(const std::string& cnt)
    {
        ne_xml_parser* parser = ne_xml_create();
        ne_xml_push_handler(parser, &startEl, &valueEl, &endEl, this);
        ne_xml_parse(parser, cnt.c_str(), cnt.size());
        ne_xml_destroy(parser);
    }

private:
    static int startEl(void* userdata, int /*parent*/, const char* /*nspace*/, const char* name, const char** attrs)
    {
        Parser* self = reinterpret_cast<Parser*>(userdata);

        NeonNode& node = self->m_current->children.emplace_back();
        node.name      = name;
        node.parent    = self->m_current;

        for (int i = 0; attrs[i] != nullptr && attrs[i + 1] != nullptr; i += 2) {
            NeonNode& attr = node.children.emplace_back();
            attr.name      = std::string("a::") + attrs[i];
            attr.value     = attrs[i + 1];
            attr.parent    = &node;
        }

        self->m_current = &node;

        return NE_XML_STATEROOT + 1;
    }

    static int valueEl(void* userdata, int /*state*/, const char* cdata, size_t len)
    {
        std::string data = trimmed(cdata, len);
        if (data.empty()) {
            return NE_XML_STATEROOT;
        }

        Parser* self = reinterpret_cast<Parser*>(userdata);

        NeonNode& cdataNode = self->m_current->children.emplace_back();
        cdataNode.name      = "cdata";
        cdataNode.value     = data;
        cdataNode.parent    = self->m_current;

        return NE_XML_STATEROOT;
    }

    static int endEl(void* userdata, int /*state*/, const char* /*nspace*/, [[maybe_unused]] const char* name)
    {
        Parser* self = reinterpret_cast<Parser*>(userdata);
        assert(self->m_current->name == name);

        self->m_current = self->m_current->parent;
        return NE_XML_STATEROOT;
    }

    static std::string trimmed(const char* data, size_t len)
    {
        return fty::trimmed(std::string(data, len));
    }

private:
    NeonNode* m_current = nullptr;
};

// =====================================================================================================================

void deserialize(const std::string& cnt, pack::Attribute& node)
{
    NeonNode neon;
    Parser   p(neon);
    p.parse(cnt);
    // neon.dump(std::cout);
    if (!neon.children.empty()) {
        NeonDeserializer::visit(node, neon.children[0]);
    }
}

} // namespace neon
