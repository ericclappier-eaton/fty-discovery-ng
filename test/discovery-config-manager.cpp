#include "discovery-config-manager.h"
#include "discovery-config.h"
#include <catch2/catch.hpp>
#include <iostream>
#include <pack/serialization.h>


TEST_CASE("Config manager load")
{
    using namespace fty::disco;

    auto manager = ConfigDiscoveryManager::instance();

    {
        auto load = manager.load("config-discovery-some.conf");
        REQUIRE(!load);
    }

    auto load = manager.load("config-discovery.conf");
    REQUIRE(load.value().aux.createUser == "somesome");
}

TEST_CASE("Config manager save/load")
{
    using namespace fty::disco;

    auto manager = ConfigDiscoveryManager::instance();

    ConfigDiscovery conf;

    conf.discovery.type = ConfigDiscovery::Discovery::Type::LOCAL;
    conf.discovery.ips.append("1.12.2.12");
    conf.aux.priority = 100;

    manager.set(conf);

    auto filled = *manager.config();
    CHECK(filled.discovery.type == ConfigDiscovery::Discovery::Type::LOCAL);
    CHECK(filled.discovery.ips[0] == "1.12.2.12");
    CHECK(filled.aux.priority == 100);

    REQUIRE(manager.save("config-discovery-tmp.conf"));

    auto load = manager.load("config-discovery-tmp.conf").value();
    CHECK(load.discovery.type == ConfigDiscovery::Discovery::Type::LOCAL);
    CHECK(load.discovery.ips[0] == "1.12.2.12");
    REQUIRE(load.aux.priority == 100);
}
