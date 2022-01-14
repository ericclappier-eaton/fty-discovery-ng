#include "discovery-config-manager.h"
#include "discovery-config.h"
#include <catch2/catch.hpp>
#include <iostream>
#include <pack/serialization.h>

TEST_CASE("Config manager command create")
{
    using namespace fty::disco;

    auto manager = ConfigDiscoveryManager::instance();

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


    REQUIRE(manager.save("config-discovery-tmp.conf"));
}

TEST_CASE("Config manager command read")
{
    using namespace fty::disco;

    auto manager = ConfigDiscoveryManager::instance();

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

    pack::StringList in;
    in.append("FTY_DISCOVERY_TYPE");
    in.append("FTY_DISCOVERY_SCANS_DISABLED");
    in.append("FTY_DISCOVERY_IPS");
    auto ret = manager.commandRead(in);

    std::stringstream local;
    local << ConfigDiscovery::Discovery::Type::Local;

    REQUIRE(ret.value().type == local.str());
    REQUIRE(ret.value().scansDisabled.empty());
    REQUIRE(ret.value().ips.size() == 1);
    REQUIRE(ret.value().ips[0] == "1.12.2.12");
}

TEST_CASE("Config manager load")
{
    using namespace fty::disco;

    auto manager = ConfigDiscoveryManager::instance();

    {
        auto load = manager.load("config-discovery-some.conf");
        REQUIRE(!load);
    }

    auto load = manager.load("config-discovery.conf");
    REQUIRE(load.value().parameters.dumpDataLoopTime == 9988);
}

TEST_CASE("Config manager save/load")
{
    using namespace fty::disco;

    auto manager = ConfigDiscoveryManager::instance();

    ConfigDiscovery conf;

    conf.discovery.type = ConfigDiscovery::Discovery::Type::Local;
    conf.discovery.ips.append("1.12.2.12");
    conf.aux.priority = 100;

    manager.set(conf);

    auto filled = *manager.config();
    CHECK(filled.discovery.type == ConfigDiscovery::Discovery::Type::Local);
    CHECK(filled.discovery.ips[0] == "1.12.2.12");
    CHECK(filled.aux.priority == 100);

    REQUIRE(manager.save("config-discovery-tmp.conf"));

    auto load = manager.load("config-discovery-tmp.conf").value();
    CHECK(load.discovery.type == ConfigDiscovery::Discovery::Type::Local);
    CHECK(load.discovery.ips[0] == "1.12.2.12");
    REQUIRE(load.aux.priority == 100);
}
