#include "discovery-config-manager.h"
#include "discovery-config.h"
#include <catch2/catch.hpp>
#include <iostream>
#include <pack/serialization.h>

TEST_CASE("Config manager command create")
{
    using namespace fty::disco;

    auto manager = ConfigDiscoveryManager::instance();
    // manager.load("config-discovery.conf");

    commands::config::Config conf;

    conf.type = "localscan";
    conf.ips.append("1.12.2.12");
    conf.priority = 100;

    manager.load("config-discovery.conf");
    manager.commandCreate(conf);

    auto filled = *manager.config();
    CHECK(filled.discovery.type == ConfigDiscovery::Discovery::Type::Local);
    CHECK(filled.discovery.ips[0] == "1.12.2.12");
    CHECK(filled.aux.priority == 100);

    conf.ips.append("1.12.2.256");

    REQUIRE(!manager.commandCreate(conf));

    conf.ips.append("aa.aa");
    REQUIRE(!manager.commandCreate(conf));
    conf.ips.append("");
    REQUIRE(!manager.commandCreate(conf));


    REQUIRE(manager.save("config-discovery.conf"));
    
}
