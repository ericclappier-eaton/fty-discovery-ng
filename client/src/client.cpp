#include "fty/discovery/client.h"
#include "commands.h"
#include "message-bus.h"

namespace fty {

// =====================================================================================================================

DiscoveryClient::DiscoveryClient(const std::string& clientName, const std::string& address)
    : m_clientName(clientName)
    , m_address(address)
{
}

DiscoveryClient::~DiscoveryClient()
{
}

Expected<DiscoveryClient::Protocols> DiscoveryClient::protocols() const
{
    fty::MessageBus bus;
    if (auto res = bus.init(m_clientName); !res) {
        return unexpected(res.error());
    }

    commands::protocols::In in;
    in.address = m_address;

    fty::Message msg;
    msg.userData.setString(*pack::json::serialize(in));
    msg.meta.to      = "discovery-ng";
    msg.meta.subject = commands::protocols::Subject;
    msg.meta.from    = m_clientName;

    if (Expected<fty::Message> resp = bus.send(fty::Channel, msg)) {
        if (resp->meta.status == fty::Message::Status::Error) {
            return unexpected(resp->userData.asString());
        }

        if (auto res = resp->userData.decode<commands::protocols::Out>()) {
            return res->value();
        } else {
            return unexpected(res.error());
        }
    } else {
        return unexpected(resp.error());
    }
}

SnmpDiscovery DiscoveryClient::snmp() const
{
    SnmpDiscovery snmp(*this);
    return snmp;
}

// =====================================================================================================================

SnmpDiscovery::SnmpDiscovery(const DiscoveryClient& discovery)
    : m_client(discovery)
{
}

void SnmpDiscovery::setPort(uint16_t port)
{
    m_port = port;
}

void SnmpDiscovery::setCredentiaId(const std::string& credentiaId)
{
    m_credentiaId = credentiaId;
}

Expected<SnmpDiscovery::Mibs> SnmpDiscovery::mibs() const
{
    fty::MessageBus bus;
    if (auto res = bus.init(m_client.m_clientName); !res) {
        return unexpected(res.error());
    }

    commands::mibs::In in;
    in.address      = m_client.m_address;
    in.port         = m_port;
    in.credentialId = m_credentiaId;

    fty::Message msg;
    msg.userData.setString(*pack::json::serialize(in));
    msg.meta.to      = "discovery-ng";
    msg.meta.subject = commands::mibs::Subject;
    msg.meta.from    = m_client.m_clientName;

    if (Expected<fty::Message> resp = bus.send(fty::Channel, msg)) {
        if (resp->meta.status == fty::Message::Status::Error) {
            return unexpected(resp->userData.asString());
        }

        if (auto res = resp->userData.decode<commands::mibs::Out>()) {
            return res->value();
        } else {
            return unexpected(res.error());
        }
    } else {
        return unexpected(resp.error());
    }
}

Expected<SnmpDiscovery::Assets> SnmpDiscovery::assets(const std::string& /*mib*/) const
{
    return unexpected("not implemented");
}

// =====================================================================================================================

} // namespace fty
