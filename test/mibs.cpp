#include "test-common.h"
#include <fty/process.h>

TEST_CASE("Mibs / Empty request")
{
    fty::Message msg = Test::createMessage(fty::commands::mibs::Subject);

    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: payload is empty" == ret.error());
}

TEST_CASE("Mibs / Wrong request")
{
    fty::Message msg = Test::createMessage(fty::commands::mibs::Subject);

    msg.userData.setString("Some shit");
    fty::Expected<fty::Message> ret = Test::send(msg);

    CHECK_FALSE(ret);
    CHECK("Wrong input data: format of payload is incorrect" == ret.error());
}

TEST_CASE("Mibs / Unaviable host")
{
    fty::Message msg = Test::createMessage(fty::commands::mibs::Subject);

    fty::commands::protocols::In in;
    in.address = "pointtosky";
    msg.userData.setString(*pack::json::serialize(in));

    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available: pointtosky" == ret.error());
}

TEST_CASE("Mibs / get mibs")
{
    // clang-format off
    fty::Process proc("/usr/bin/python3",  {
        "/usr/bin/snmpsimd",
        "--data-dir=root",
        "--agent-udpv4-endpoint=127.0.0.1:1161",
        "--logging-method=file:.snmpsim.txt",
        "--variation-modules-dir=root"/*,
        "--log-level=error"*/
    });
    // clang-format on

    auto getResponse = [](const fty::commands::mibs::In& in) {
        fty::Message msg = Test::createMessage(fty::commands::mibs::Subject);
        msg.userData.setString(*pack::json::serialize(in));

        fty::Expected<fty::Message> ret = Test::send(msg);
        if (!ret) {
            FAIL(ret.error());
        }
        REQUIRE(ret);

        fty::Expected<fty::commands::mibs::Out> res = ret->userData.decode<fty::commands::mibs::Out>();
        CHECK(res);
        CHECK(res->size());
        return *res;
    };

    if (auto pid = proc.run()) {
        fty::commands::mibs::In in;
        in.address = "127.0.0.1";
        in.port    = 1161;

        SECTION("Daisy device epdu.147")
        {
            in.community = "epdu.147";
            auto res = getResponse(in);
            CHECK("EATON-EPDU-MIB::eatonEpdu" == res[0]);
        }

        SECTION("MG device mge.125")
        {
            in.community = "mge.125";
            auto res = getResponse(in);
            CHECK("MG-SNMP-UPS-MIB::upsmg" == res[0]);
        }

        SECTION("MG device mge.191")
        {
            in.community = "mge.191";
            auto res = getResponse(in);
            CHECK("MG-SNMP-UPS-MIB::upsmg" == res[0]);
        }

        SECTION("HP device cpqpqwer.114")
        {
            in.community = "cpqpqwer.114";
            auto res = getResponse(in);
            CHECK("CPQPOWER-MIB::ups" == res[0]);
        }

        SECTION("Lenovo device lenovo.181")
        {
            in.community = "lenovo.181";
            auto res = getResponse(in);
            CHECK("joint-iso-ccitt" == res[0]);
        }

        SECTION("Genapi device xups.238")
        {
            in.community = "xups.238";
            auto res = getResponse(in);
            CHECK("EATON-OIDS::xupsMIB" == res[0]);
        }

        SECTION("Genapi device xups.159")
        {
            in.community = "xups.159";
            auto res = getResponse(in);
            CHECK("EATON-OIDS::xupsMIB" == res[0]);
        }

        proc.interrupt();
        proc.wait();
        //std::cerr << proc.readAllStandardError() << std::endl;
    } else {
        FAIL(pid.error());
    }
}
