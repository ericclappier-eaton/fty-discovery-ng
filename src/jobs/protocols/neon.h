#pragma once
#include <fty/expected.h>
#include <memory>

struct ne_session_s;
typedef struct ne_session_s ne_session;

namespace pack {
class Attribute;
}

namespace neon {

class Neon
{
public:
    Neon();
    ~Neon();

    fty::Expected<std::string> get(const std::string& path);
    bool connect(const std::string& address, uint16_t port);
private:
    static void closeSession(ne_session*);
private:
    using NeonSession = std::unique_ptr<ne_session, decltype(&closeSession)>;
private:
    NeonSession m_session;
};

void deserialize(const std::string& cnt, pack::Attribute& node);

}

