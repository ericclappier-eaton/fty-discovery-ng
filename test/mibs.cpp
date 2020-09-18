#include "test-common.h"
#include <fty/process.h>

TEST_CASE("Mibs / Empty request")
{
    fty::Message msg = Test::createMessage(fty::commands::mibs::Subject);

    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data" == ret.error());
}

TEST_CASE("Mibs / Wrong request")
{
    fty::Message msg = Test::createMessage(fty::commands::mibs::Subject);

    msg.userData.setString("Some shit");
    fty::Expected<fty::Message> ret = Test::send(msg);

    CHECK_FALSE(ret);
    CHECK("Wrong input data" == ret.error());
}

TEST_CASE("Mibs / Unaviable host")
{
    fty::Message msg = Test::createMessage(fty::commands::mibs::Subject);

    fty::commands::protocols::In in;
    in.address = "pointtosky";
    msg.userData.setString(*pack::json::serialize(in));

    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available" == ret.error());
}

TEST_CASE("Mibs / get mibs")
{
    // clang-format off
    fty::Process proc("snmpsimd",  {
        "--data-dir=root",
        "--agent-udpv4-endpoint=127.0.0.1:1161",
        "--logging-method=file:.snmpsim.txt",
        "--variation-modules-dir=root"//,
        //"--log-level=error"
    });
    // clang-format on

    if (auto pid = proc.run()) {
        fty::Message msg = Test::createMessage(fty::commands::mibs::Subject);

        fty::commands::mibs::In in;
        in.address = "127.0.0.1";
        in.port    = 1161;

        SECTION("Daisy device 38.147")
        {
            in.community = "10.130.38.147";
        }

        SECTION("MG device 38.125")
        {
            in.community = "10.130.38.125";
        }

        SECTION("MG device 38.191")
        {
            in.community = "10.130.38.191";
        }

        SECTION("HP device 38.114")
        {
            in.community = "10.130.38.114";
        }

        SECTION("Lenovo device 38.181")
        {
            in.community = "10.130.38.181";
        }

        SECTION("Genapi device 38.238")
        {
            in.community = "10.130.38.238";
        }

        SECTION("Genapi device 38.159")
        {
            in.community = "10.130.38.159";
        }

        msg.userData.setString(*pack::json::serialize(in));
        fty::Expected<fty::Message> ret = Test::send(msg);
        CHECK(ret);

        proc.interrupt();
        proc.wait();
    } else {
        FAIL(pid.error());
    }
}
