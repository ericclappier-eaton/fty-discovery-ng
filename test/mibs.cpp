#include "test-common.h"
#include <fty/process.h>

namespace fty::disco {

TEST_CASE("Mibs / Empty request", "[mibs]")
{
    fty::disco::Message msg = Test::createMessage(commands::mibs::Subject);

    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: payload is empty" == ret.error());
}

TEST_CASE("Mibs / Wrong request", "[mibs]")
{
    fty::disco::Message msg = Test::createMessage(commands::mibs::Subject);

    msg.userData.setString("Some shit");
    fty::Expected<fty::disco::Message> ret = Test::send(msg);

    CHECK_FALSE(ret);
    CHECK("Wrong input data: format of payload is incorrect" == ret.error());
}

TEST_CASE("Mibs / Unaviable host", "[mibs]")
{
    fty::disco::Message msg = Test::createMessage(commands::mibs::Subject);

    commands::protocols::In in;
    in.address = "pointtosky";
    msg.userData.setString(*pack::json::serialize(in));

    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available: pointtosky" == ret.error());
}

TEST_CASE("Mibs / get mibs", "[mibs]")
{
    // clang-format off
    fty::Process proc("snmpsimd", {
        "--data-dir=root",
        "--agent-udpv4-endpoint=127.0.0.1:1161",
        "--logging-method=file:.snmpsim.txt",
        "--variation-modules-dir=root",
        "--log-level=error"
    });
    // clang-format on

    auto getResponse = [](const commands::mibs::In& in) {
        fty::disco::Message msg = Test::createMessage(commands::mibs::Subject);
        msg.userData.setString(*pack::json::serialize(in));

        fty::Expected<fty::disco::Message> ret = Test::send(msg);
        if (!ret) {
            FAIL(ret.error());
        }
        REQUIRE(ret);

        fty::Expected<commands::mibs::Out> res = ret->userData.decode<commands::mibs::Out>();
        CHECK(res);
        CHECK(res->size());
        return *res;
    };

    if (auto pid = proc.run()) {
        commands::mibs::In in;
        in.address = "127.0.0.1";
        in.port    = 1161;
        in.timeout = 5000;

        SECTION("Daisy device epdu.147")
        {
            in.community = "epdu.147";
            auto res     = getResponse(in);
            CHECK("EATON-EPDU-MIB::eatonEpdu" == res[0]);
        }

        SECTION("MG device mge.125")
        {
            in.community = "mge.125";
            auto res     = getResponse(in);
            CHECK("MG-SNMP-UPS-MIB::upsmg" == res[0]);
        }

        SECTION("MG device mge.191")
        {
            in.community = "mge.191";
            auto res     = getResponse(in);
            CHECK("MG-SNMP-UPS-MIB::upsmg" == res[0]);
        }

        SECTION("HP device cpqpqwer.114")
        {
            in.community = "cpqpqwer.114";
            auto res     = getResponse(in);
            CHECK("CPQPOWER-MIB::ups" == res[0]);
        }

        SECTION("Lenovo device lenovo.181")
        {
            in.community = "lenovo.181";
            auto res     = getResponse(in);
            CHECK("joint-iso-ccitt" == res[0]);
        }

        SECTION("Genapi device xups.238")
        {
            in.community = "xups.238";
            auto res     = getResponse(in);
            CHECK("EATON-OIDS::xupsMIB" == res[0]);
        }

        SECTION("Genapi device xups.159")
        {
            in.community = "xups.159";
            auto res     = getResponse(in);
            CHECK("EATON-OIDS::xupsMIB" == res[0]);
        }

        // std::cerr << proc.readAllStandardError() << std::endl;
        proc.interrupt();
        proc.wait();
    } else {
        FAIL(pid.error());
    }
}

} // namespace fty::disco
