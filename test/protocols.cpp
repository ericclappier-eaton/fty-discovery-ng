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

TEST_CASE("Protocols / Unaviable host", "[protocols]")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    commands::protocols::In in;
    in.address = "pointtosky.roz.lab.etn.com";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available: pointtosky.roz.lab.etn.com" == ret.error());
}

TEST_CASE("Protocols / Not asset", "[protocols]")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    commands::protocols::In in;
    in.address = "127.0.0.1";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK(ret);
    auto res = ret->userData.decode<commands::protocols::Out>();
    CHECK(res);
    CHECK(0 == res->size());
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

        commands::protocols::In in;
        in.address = "127.0.0.1";
        in.port = 1161;

        fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);
        msg.userData.setString(*pack::json::serialize(in));

        fty::Expected<fty::disco::Message> ret = Test::send(msg);

        CHECK(ret);
        auto res = ret->userData.decode<commands::protocols::Out>();
        CHECK(res);
        CHECK(1 == res->size());
        // TBD_MERGE
        CHECK("nut_snmp" == (*res)[0].protocol);
        CHECK(161 == (*res)[0].port);
        CHECK(true == (*res)[0].reachable);

        // TBD_MERGE
        //CHECK("nut_xml_pdc" == (*res)[1].protocol);
        //CHECK(4321 == (*res)[1].port);
        //CHECK(false == (*res)[1].reachable);

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

TEST_CASE("Protocols / getProtocolStr", "[protocols]")
{
    auto res = fty::disco::job::Protocols::getProtocolStr(fty::disco::job::Type::Powercom);
    CHECK(res == "nut_powercom");
    res = fty::disco::job::Protocols::getProtocolStr(fty::disco::job::Type::Xml);
    CHECK(res == "nut_xml_pdc");
    res = fty::disco::job::Protocols::getProtocolStr(fty::disco::job::Type::Snmp);
    CHECK(res == "nut_snmp");
}

TEST_CASE("Protocols / splitPortFromProtocol", "[protocols]")
{
    auto res = fty::disco::job::Protocols::splitPortFromProtocol("");
    CHECK(res.first == "");
    CHECK(res.second == "");
    res = fty::disco::job::Protocols::splitPortFromProtocol("nut_snmp");
    CHECK(res.first == "nut_snmp");
    CHECK(res.second == "");
    res = fty::disco::job::Protocols::splitPortFromProtocol("nut_snmp:162");
    CHECK(res.first == "nut_snmp");
    CHECK(res.second == "162");
}

TEST_CASE("Protocols / getPort", "[protocols]")
{
    fty::disco::commands::protocols::In in;
    auto res = fty::disco::job::Protocols::getPort("", in);
    CHECK(res == std::nullopt);
    in.port = 162;
    res = fty::disco::job::Protocols::getPort("", in);
    CHECK(*res == 162);
    in.protocols.append("nut_snmp:163");
    res = fty::disco::job::Protocols::getPort("", in);
    CHECK(*res == 162);
    res = fty::disco::job::Protocols::getPort("nut_snmp", in);
    CHECK(*res == 162);
    in.port = 0;
    res = fty::disco::job::Protocols::getPort("nut_snmp", in);
    CHECK(*res == 163);
    res = fty::disco::job::Protocols::getPort("nut_bad", in);
    CHECK(res == std::nullopt);
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
