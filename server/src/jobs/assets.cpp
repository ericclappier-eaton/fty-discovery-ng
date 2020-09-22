#include "assets.h"
#include "commands.h"
#include "common.h"
#include "message-bus.h"
#include "protocols/snmp.h"
#include <fty_log.h>
#include "protocols/ping.h"
#include "protocols/nut.h"

namespace fty::job {

// =====================================================================================================================

namespace response {

    /// Response wrapper
    class Assets : public BasicResponse<Assets>
    {
    public:
        commands::assets::Out assets = FIELD("assets");

    public:
        using BasicResponse::BasicResponse;
        META_BASE(Assets, BasicResponse<Assets>, assets);

    public:
        const commands::assets::Out& data()
        {
            return assets;
        }
    };

} // namespace response

// =====================================================================================================================

Assets::Assets(const Message& in, MessageBus& bus)
    : m_in(in)
    , m_bus(&bus)
{
}

void Assets::operator()()
{
    response::Assets response;

    if (m_in.userData.empty()) {
        response.setError("Wrong input data");
        if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
            log_error(res.error().c_str());
        }
        return;
    }

    auto cmd = m_in.userData.decode<commands::assets::In>();
    if (!cmd) {
        response.setError("Wrong input data");
        if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
            log_error(res.error().c_str());
        }
        return;
    }

    if (!available(cmd->address)) {
        response.setError("Host is not available");
        if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
            log_error(res.error().c_str());
        }
        return;
    }

    if (auto res = protocol::properties(*cmd); !res) {
        response.setError(res.error());
        if (auto answ = m_bus->reply(fty::Channel, m_in, response); !answ) {
            log_error(answ.error().c_str());
        }
        return;
    } else {
        std::string out = *pack::json::serialize(*res);
        log_error("output %s", out.c_str());

        response.status = Message::Status::Ok;
        response.assets = *res;
        if (auto repl = m_bus->reply(fty::Channel, m_in, response); !res) {
            log_error(res.error().c_str());
        }
    }
}

} // namespace fty::job
