#include "test-common.h"
#include "../server/src/jobs/protocols.h"
#include <fty/process.h>

namespace fty::disco {

TEST_CASE("Protocols/ Empty request", "[protocols]")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: payload is empty" == ret.error());
}

TEST_CASE("Protocols / Wrong request", "[protocols]")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    msg.userData.setString("Some shit");
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: format of payload is incorrect" == ret.error());
}

TEST_CASE("Protocols / Unavailable host", "[protocols]")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    commands::protocols::In in;
    in.address = "pointtosky.roz.lab.etn.com";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available: pointtosky.roz.lab.etn.com" == ret.error());
}

TEST_CASE("Protocols / available attribute", "[protocols]")
{
    using namespace commands::protocols;

    std::stringstream ss;
    auto clearBuffer = [&ss]() {
        ss.clear();
        ss.str("");
    };

    Return::Available available;

    // serialisation
    {
        ss << available;
        CHECK("unknown" == ss.str());

        available = Return::Available::No;
        clearBuffer();
        ss << available;
        CHECK("no" == ss.str());

        available = Return::Available::Yes;
        clearBuffer();
        ss << available;
        CHECK("yes" == ss.str());

        available = Return::Available::Maybe;
        clearBuffer();
        ss << available;
        CHECK("maybe" == ss.str());
    }

    // de-serialisation
    {
        clearBuffer();
        ss.str("");
        ss >> available;
        CHECK(available == Return::Available::Unknown);

        clearBuffer();
        ss.str("not-defined");
        ss >> available;
        CHECK(available == Return::Available::Unknown);

        clearBuffer();
        ss.str("unknown");
        ss >> available;
        CHECK(available == Return::Available::Unknown);

        clearBuffer();
        ss.str("no");
        ss >> available;
        CHECK(available == Return::Available::No);

        clearBuffer();
        ss.str("maybe");
        ss >> available;
        CHECK(available == Return::Available::Maybe);

        clearBuffer();
        ss.str("yes");
        ss >> available;
        CHECK(available == Return::Available::Yes);
    }
}

TEST_CASE("Protocols / Not asset", "[protocols]")
{
    using namespace commands::protocols;

    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    commands::protocols::In in;
    in.address = "127.0.0.1";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK(ret);
    auto res = ret->userData.decode<commands::protocols::Out>();
    CHECK(res);
    CHECK(3 == res->size());
    CHECK("nut_powercom" == (*res)[0].protocol);
    CHECK(443 == (*res)[0].port);
    CHECK(false == (*res)[0].reachable);
    CHECK(Return::Available::No == (*res)[0].available);

    CHECK("nut_xml_pdc" == (*res)[1].protocol);
    CHECK(80 == (*res)[1].port);
    CHECK(false == (*res)[1].reachable);
    CHECK(Return::Available::No == (*res)[1].available);

    CHECK("nut_snmp" == (*res)[2].protocol);
    CHECK(161 == (*res)[2].port);
    CHECK(false == (*res)[2].reachable);
    CHECK(Return::Available::No == (*res)[2].available);
}

TEST_CASE("Protocols / Invalid ip", "[protocols]")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    commands::protocols::In in;
    in.address = "127.0.145.256";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available: 127.0.145.256" == ret.error());
}

TEST_CASE("Protocols / Fake request", "[protocols]")
{
    using namespace commands::protocols;

    // clang-format off
    fty::Process proc("snmpsimd", {
        "--data-dir=root",
        "--agent-udpv4-endpoint=127.0.0.1:1161",
        "--logging-method=file:.snmpsim.txt",
        "--variation-modules-dir=root",
    });
    // clang-format on

    if (auto pid = proc.run()) {

        // Wait a moment for snmpsim init
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // with the port defined in protocol
        {
            commands::protocols::In in;
            in.address = "127.0.0.1";
            ConfigDiscovery::Protocol nutPowercom;
            nutPowercom.protocol = ConfigDiscovery::Protocol::Type::Powercom;
            //nutPowercom.ports.append(4443);
            nutPowercom.port = 4443;
            in.protocols.append(nutPowercom);
            ConfigDiscovery::Protocol nutXmlPdc;
            nutXmlPdc.protocol = ConfigDiscovery::Protocol::Type::XmlPdc;
            //nutXmlPdc.ports.append(8080);
            nutXmlPdc.port = 8080;
            in.protocols.append(nutXmlPdc);
            ConfigDiscovery::Protocol nutSnmp;
            nutSnmp.protocol = ConfigDiscovery::Protocol::Type::Snmp;
            //nutSnmp.ports.append(1161);
            nutSnmp.port = 1161;
            in.protocols.append(nutSnmp);  // Only this one will match

            fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);
            msg.userData.setString(*pack::json::serialize(in));

            fty::Expected<fty::disco::Message> ret = Test::send(msg);

            CHECK(ret);
            auto res = ret->userData.decode<commands::protocols::Out>();
            CHECK(res);
            CHECK(3 == res->size());

            CHECK("nut_powercom" == (*res)[0].protocol);
            CHECK(4443  == (*res)[0].port);
            CHECK(false == (*res)[0].reachable);
            CHECK(Return::Available::No == (*res)[0].available);

            CHECK("nut_xml_pdc" == (*res)[1].protocol);
            CHECK(8080  == (*res)[1].port);
            CHECK(false == (*res)[1].reachable);
            CHECK(Return::Available::No == (*res)[1].available);

            CHECK("nut_snmp" == (*res)[2].protocol);
            CHECK(1161 == (*res)[2].port);
            CHECK(true == (*res)[2].reachable);
            CHECK(Return::Available::Maybe == (*res)[2].available);
        }

        proc.interrupt();
        proc.wait();
    } else {
        FAIL(pid.error());
    }
}

TEST_CASE("Protocols / resolve", "[protocols]")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    commands::protocols::In in;
    in.address = "127.0.0.1";
    msg.userData.setString(*pack::json::serialize(in));

    fty::Expected<fty::disco::Message> ret = Test::send(msg);

    commands::protocols::In in2;
    in2.address = "10.130.33.178";
    msg.userData.setString(*pack::json::serialize(in2));

    fty::Expected<fty::disco::Message> ret2 = Test::send(msg);
}

TEST_CASE("Protocols / findProtocol", "[protocols]")
{
    fty::disco::commands::protocols::In in;
    {
        auto res = fty::disco::job::Protocols::findProtocol(ConfigDiscovery::Protocol::Type::Unknown, in);
        CHECK(res == std::nullopt);
    }
    ConfigDiscovery::Protocol nutSnmp;
    nutSnmp.protocol = ConfigDiscovery::Protocol::Type::Snmp;
    nutSnmp.port = 163;
    //nutSnmp.ports.append(163);
    in.protocols.append(nutSnmp);
    {
        auto res = fty::disco::job::Protocols::findProtocol(ConfigDiscovery::Protocol::Type::Unknown, in);
        CHECK(res == std::nullopt);
    }
    {
        auto res = fty::disco::job::Protocols::findProtocol(ConfigDiscovery::Protocol::Type::Snmp, in);
        CHECK(res->protocol == ConfigDiscovery::Protocol::Type::Snmp);
        //CHECK(res->ports[0] == 163);
        CHECK(res->port == 163);
    }
    {
        auto res = fty::disco::job::Protocols::findProtocol(ConfigDiscovery::Protocol::Type::Unknown, in);
        CHECK(res == std::nullopt);
    }
}

/*TEST_CASE("Protocols / powercom")
{
    fty::Message msg = Test::createMessage(fty::commands::protocols::Subject);

    fty::commands::protocols::In in;
    in.address = "10.130.33.34";
    msg.userData.setString(*pack::json::serialize(in));

    fty::Expected<fty::Message> ret = Test::send(msg);

    CHECK(ret);
    auto res = ret->userData.decode<fty::commands::protocols::Out>();
    CHECK(res);
    CHECK(2 == res->size());
    CHECK("nut_powercom" == (*res)[0]);
}*/

} // namespace fty::disco
