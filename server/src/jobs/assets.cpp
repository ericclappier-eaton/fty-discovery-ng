#include "assets.h"
#include "commands.h"
#include "common.h"
#include "message-bus.h"
#include "protocols/snmp.h"
#include <fty_log.h>
#include "protocols/ping.h"

namespace fty::job {

namespace response {

    /// Response wrapper
    class Assets : public BasicResponse<Assets>
    {
    public:
        commands::assets::Out assets = FIELD("assets");

    public:
        using BasicResponse::BasicResponse;
        META(Assets, assets);

    public:
        const commands::assets::Out& data()
        {
            return assets;
        }
    };

} // namespace response

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

    log_error("check addr %s", cmd->address.value().c_str());
    if (!available(cmd->address)) {
        response.setError("Host is not available");
        if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
            log_error(res.error().c_str());
        }
        return;
    }

    log_error(cmd->address.value().c_str());


    static std::map<std::string, std::string> snmpMap = {
        {"UPS-MIB::upsIdentManufacturer.0", "manufacturer"},
        {"UPS-MIB::upsIdentModel.0", "model"},
        {"UPS-MIB::upsIdentName.0", "uuid"},
        {"RFC1213-MIB::ipAdEntAddr", "ip"},
    };

    auto session = protocol::Snmp::instance().session(cmd->address);
    if (auto res = session->open(); !res) {
        response.setError(res.error());
        log_error(res.error().c_str());
        if (auto answ = m_bus->reply(fty::Channel, m_in, response); !answ) {
            log_error(answ.error().c_str());
        }
        return;
    }

    auto name = session->read("SNMPv2-MIB::sysDescr.0");
    if (!name) {
        response.setError(name.error());
        log_error(name.error().c_str());
        if (auto answ = m_bus->reply(fty::Channel, m_in, response); !answ) {
            log_error(answ.error().c_str());
        }
        return;
    }

    response.assets.ip = cmd->address;
    auto fields = response.assets.fields();
    for (const auto& pair : snmpMap) {
        if (auto val = session->read(pair.first); !val) {
            log_error(val.error().c_str());
            continue;
        } else {
            auto it = std::find_if(fields.begin(), fields.end(), [&](const auto* attr) {
                return attr->key() == pair.second;
            });

            if (it != fields.end()) {
                pack::String* vfld = dynamic_cast<pack::String*>(*it);
                vfld->setValue(*val);
            }
        }
    }


    response.status = Message::Status::Ok;
    if (auto res = m_bus->reply(fty::Channel, m_in, response); !res) {
        log_error(res.error().c_str());
    }
}

} // namespace fty::job
