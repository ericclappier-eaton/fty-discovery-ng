#include "test-common.h"
#include <fty/process.h>

TEST_CASE("Assets / Empty request")
{
    fty::Message                msg = Test::createMessage(fty::commands::assets::Subject);
    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data" == ret.error());
}

TEST_CASE("Assets / Wrong request")
{
    fty::Message msg = Test::createMessage(fty::commands::assets::Subject);

    msg.userData.setString("Some shit");
    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data" == ret.error());
}

TEST_CASE("Assets / Unaviable host")
{
    fty::Message msg = Test::createMessage(fty::commands::assets::Subject);

    fty::commands::protocols::In in;
    in.address = "pointtosky";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available" == ret.error());
}

TEST_CASE("Assets / Test output")
{
    // clang-format off
    fty::Process proc("snmpsimd",  {
        "--data-dir=assets",
        "--agent-udpv4-endpoint=127.0.0.1:1161",
        "--logging-method=file:.snmpsim.txt",
        "--variation-modules-dir=assets",
        "--log-level=error"
    });
    // clang-format on

    if (auto pid = proc.run()) {
        fty::Message msg = Test::createMessage(fty::commands::assets::Subject);

        fty::commands::assets::In in;
        in.address = "127.0.0.1";
        in.port    = 1161;
        in.driver  = "snmp-ups";

        SECTION("Daisy device epdu.147")
        {
            in.community = "epdu.147";
        }

        SECTION("MG device mge.125")
        {
            in.community = "mge.125";
        }

        SECTION("MG device mge.191")
        {
            in.community = "mge.191";
        }

        SECTION("HP device cpqpqwer.114")
        {
            in.community = "cpqpqwer.114";
        }

        SECTION("Lenovo device lenovo.181")
        {
            in.community = "lenovo.181";
        }

        SECTION("Genapi device xups.238")
        {
            in.community = "xups.238";
        }

        SECTION("Genapi device xups.159")
        {
            in.community = "xups.159";
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
