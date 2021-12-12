#include <catch2/catch.hpp>
#include <iostream>
#include <string>
#include "discovery-config.h"
#include "commands.h"
#include <pack/serialization.h>

TEST_CASE("Config save as zconfig")
{
    /* fty::disco::ConfigDiscovery cd;
    cd.server.timeout    = 10000;
    cd.server.background = 0;
    cd.server.workdir    = ".";
    cd.server.verbose    = 0;

    cd.link.src   = "0";
    cd.link.type = 1; */


    // std::cerr << *pack::zconfig::serialize(cd, pack::Option::ValueAsString | pack::Option::WithDefaults) << std::endl;
    // cd.save("disco-conf.conf");
    // fty::disco::zproject::saveToFile(cd, "disco-conf.conf");
    /* if (auto info = pack::zconfig::deserializeFile("/etc/fty-discovery/fty-discovery.cfg", cd); !info) {
        REQUIRE(false);
    }

    // auto get = pack::zconfig::deserializeFile(std::string("/etc/fty-discovery/fty-discovery.cfg"), cd);

    std::cerr << *pack::zconfig::serialize(cd, pack::Option::ValueAsString | pack::Option::WithDefaults);
 */

    fty::disco::commands::config::In in;
    std::string json(R"(
        {"FTY_DISCOVERY_TYPE":"fullscan",
        "FTY_DISCOVERY_IPS":["10.130.33.34"],
        "FTY_DISCOVERY_IPS_DISABLED":[],
        "FTY_DISCOVERY_SCANS":[],
        "FTY_DISCOVERY_SCANS_DISABLED":[],
        "FTY_DISCOVERY_DOCUMENTS":["4b8407f1-bb13-44c2-8099-a631ea61e343"],
        "FTY_DISCOVERY_DEFAULT_VALUES_STATUS":"active",
        "FTY_DISCOVERY_DEFAULT_VALUES_PRIORITY":3,
        "FTY_DISCOVERY_DEFAULT_VALUES_PARENT":"datacenter-42276389",
        "FTY_DISCOVERY_DEFAULT_VALUES_LINK_SRC":"feed-39742527"}
    )");

    if(auto ret = pack::json::deserialize(json, in); !ret){
        REQUIRE(false);
    }
    CHECK(!in.dumpPool.hasValue());
    CHECK(in.priority.value() == 3);

    fty::disco::ConfigDiscovery cd;
    fty::disco::ConfigDiscovery cd1;

    std::cerr << in.type.key();

    if (in.type.hasValue()) {
        std::stringstream                ss(in.type.value());
        fty::disco::ConfigDiscovery::Discovery::Type tmpType;
        ss >> tmpType;
        cd.discovery.type = tmpType;
    }
    if (in.scans.hasValue()) {
        cd.discovery.scans = in.scans;
    }
    if (in.ips.hasValue()) {
        cd.discovery.ips = in.ips;
    }
    if (in.docs.hasValue()) {
        cd.discovery.documents = in.docs;
    }
    if (in.status.hasValue()) {
        cd.aux.status = in.status;
    }
    if (in.priority.hasValue()) {
        cd.aux.priority = in.priority;
    }
    if (in.parent.hasValue()) {
        cd.aux.parent = in.parent;
    }
    if (in.linkSrc.hasValue()) {
        cd.link.src = in.linkSrc;
    }
    if (in.scansDisabled.hasValue()) {
        cd.disabled.scans = in.scansDisabled;
    }
    if (in.ipsDisabled.hasValue()) {
        cd.disabled.ips = in.ipsDisabled;
    }
    if (in.protocols.hasValue()) {
        cd.discovery.protocols = in.protocols;
    }
    if (in.dumpPool.hasValue()) {
        cd.parameters.maxDumpPoolNumber = in.dumpPool;
    }
    if (in.scanPool.hasValue()) {
        cd.parameters.maxScanPoolNumber = in.scanPool;
    }
    if (in.scanTimeout.hasValue()) {
        cd.parameters.nutScannerTimeOut = in.scanTimeout;
    }
    if (in.dumpLooptime.hasValue()) {
        cd.parameters.dumpDataLoopTime = in.dumpLooptime;
    }

    CHECK(cd.aux.priority == 3);

    REQUIRE(true);
}