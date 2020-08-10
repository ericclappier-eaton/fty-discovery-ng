#pragma once
#include <fty/expected.h>
#include <string>
#include <vector>

namespace fty {

class DiscoveryClient;

class SnmpDiscovery
{
public:
    struct Asset
    {
        const std::string name;
    };

    using Mibs   = std::vector<std::string>;
    using Assets = std::vector<Asset>;

public:
    void setPort(uint16_t port);
    void setCredentiaId(const std::string& credentiaId);

    [[nodiscard]] Expected<Mibs>   mibs() const;
    [[nodiscard]] Expected<Assets> assets(const std::string& mib) const;

private:
    friend class DiscoveryClient;
    SnmpDiscovery(const DiscoveryClient& discovery);

private:
    const DiscoveryClient& m_client;
    uint16_t               m_port = 161;
    std::string            m_credentiaId;
};

class DiscoveryClient
{
public:
    using Protocols = std::vector<std::string>;

public:
    DiscoveryClient(const std::string& clientName, const std::string& address);
    ~DiscoveryClient();

    [[nodiscard]] Expected<Protocols> protocols() const;
    [[nodiscard]] SnmpDiscovery       snmp() const;

private:
    friend class SnmpDiscovery;
    std::string m_clientName;
    std::string m_address;
};

} // namespace fty
