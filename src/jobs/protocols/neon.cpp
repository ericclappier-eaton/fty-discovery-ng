#include "neon.h"
#include <iostream>
#include <neon/ne_request.h>
#include <neon/ne_session.h>
#include <neon/ne_xml.h>
#include <pack/visitor.h>

namespace neon {

struct Request
{
    Request(ne_request* req)
        : m_request(req, &ne_request_destroy)
    {
        ne_begin_request(m_request.get());
    }

    ~Request()
    {
        ne_end_request(m_request.get());
    }

    const ne_status* status() const
    {
        return ne_get_status(m_request.get());
    }

    ne_request* get()
    {
        return m_request.get();
    }

private:
    using NeonRequest = std::unique_ptr<ne_request, decltype(&ne_request_destroy)>;

    NeonRequest m_request;
};

Neon::Neon()
    : m_session(nullptr, &closeSession)
{
}

Neon::~Neon()
{
}

fty::Expected<std::string> Neon::get(const std::string& path)
{
    static constexpr size_t SIZE = 1024;
    Request                 request(ne_request_create(m_session.get(), "GET", ("/" + path).c_str()));

    auto status = request.status();

    if (status->code == 200) {
        std::string body;

        std::string chunk;
        chunk.resize(SIZE);

        ssize_t bytes = 0;
        while ((bytes = ne_read_response_block(request.get(), chunk.data(), 1024)) > 0) {
            body += std::string(chunk, 0, size_t(bytes));
        }
        return body;
    } else {
        return fty::unexpected() << status->code << " " << status->reason_phrase;
    }
}

bool Neon::connect(const std::string& address, uint16_t port)
{
    m_session = NeonSession(ne_session_create("http", address.c_str(), port), &closeSession);
    return m_session != nullptr;
}

void Neon::closeSession(ne_session* sess)
{
    if (sess) {
        ne_session_destroy(sess);
    }
}

// =====================================================================================================================

struct NeonNode
{
    std::string           name;
    std::string           value;
    std::vector<NeonNode> children;
    NeonNode*             parent = nullptr;
};

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

    static void unpackValue(pack::IObjectList& /*list*/, const NeonNode& /*neon*/)
    {
    }

    static void unpackValue(pack::IProtoMap& /*map*/, const NeonNode& /*neon*/)
    {
    }
};

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
        Parser* self = reinterpret_cast<Parser*>(userdata);
        self->m_current->value = std::string(cdata, len);
        return NE_XML_STATEROOT;
    }

    static int endEl(void* userdata, int /*state*/, const char* /*nspace*/, const char* name)
    {
        Parser* self = reinterpret_cast<Parser*>(userdata);
        assert(self->m_current->name == name);

        self->m_current = self->m_current->parent;
        return NE_XML_STATEROOT;
    }

private:
    NeonNode* m_current = nullptr;
};

// =====================================================================================================================

void dump(const NeonNode& node, size_t level = 0)
{
    std::string indent(level * 4, ' ');
    if (node.parent == nullptr) {
        std::cerr << indent << "[root]\n";
    } else {
        std::cerr << indent << "[" << node.name << "]";
        if (!node.value.empty()) {
            std::cerr << " = " << node.value;
        }
        std::cerr << "\n";
    }
    for (const auto& child : node.children) {
        dump(child, level + 1);
    }
}

void deserialize(const std::string& cnt, pack::Attribute& node)
{
    NeonNode neon;
    Parser   p(neon);
    p.parse(cnt);
    dump(neon);
    NeonDeserializer::visit(node, neon.children[0]);
}

} // namespace neon
