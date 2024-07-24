#include "test-common.h"
#include <fty/process.h>
#include <fty_log.h>
#include <iostream>
#include <iomanip>
#include <fstream>


TEST_CASE("Assets / Empty request", "[assets]")
{
    fty::disco::Message                msg = Test::createMessage(fty::disco::commands::assets::Subject);
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: payload is empty" == ret.error());
}

TEST_CASE("Assets / Wrong request", "[assets]")
{
    fty::disco::Message msg = Test::createMessage(fty::disco::commands::assets::Subject);

    msg.userData.setString("Some shit");
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: format of payload is incorrect" == ret.error());
}

TEST_CASE("Assets / Unaviable host", "[assets]")
{
    fty::disco::Message msg = Test::createMessage(fty::disco::commands::assets::Subject);

    fty::disco::commands::protocols::In in;
    in.address = "pointtosky";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available: pointtosky" == ret.error());
}

TEST_CASE("Assets / Test output", "[assets]")
{
    // clang-format off
    fty::Process proc("snmpsimd",  {
        "--data-dir=assets",
        "--agent-udpv4-endpoint=127.0.0.1:1161",
        "--logging-method=file:.snmpsim.txt",
        "--variation-modules-dir=assets"
        //"--log-level=error"
    });
    // clang-format on

    if (auto pid = proc.run()) {
        // Wait a moment for snmpsim init
        std::this_thread::sleep_for(std::chrono::seconds(1));

        ///////////////////////////////////////////////////////////////
        // Test
        //system("ps aux | grep snmpsimd");
        int status = std::system("ps aux | grep snmpsimd > test.txt"); 
        std::cout << "Exit code: " << WEXITSTATUS(status) << std::endl;

        std::ifstream ifs("test.txt", std::ofstream::binary);
        if (ifs.is_open())
        {
            std::stringstream buf;
            buf << ifs.rdbuf();
            std::cout << "buf=" << buf.str() << std::endl;
            logInfo("buff={}", buf.str());
        }
        else {
           std::cout << "Error when reading test file" << std::endl; 
           logError("Error when reading test file");
        }
        ///////////////////////////////////////////////////////////////

        fty::disco::Message msg = Test::createMessage(fty::disco::commands::assets::Subject);

        fty::disco::commands::assets::In in;
        in.address          = "127.0.0.1";
        in.port             = 1161;
        in.protocol         = "nut_snmp";
        in.settings.timeout = 10000;

        SECTION("Daisy device epdu.147")
        {
            in.settings.mib       = "EATON-EPDU-MIB::eatonEpdu";
            in.settings.community = "epdu.147";
        }

        SECTION("MG device mge.125")
        {
            in.settings.mib       = "MG-SNMP-UPS-MIB::upsmg";
            in.settings.community = "mge.125";
        }

        SECTION("MG device mge.191")
        {
            in.settings.mib       = "MG-SNMP-UPS-MIB::upsmg";
            in.settings.community = "mge.191";
        }

        /*SECTION("HP device cpqpqwer.114")
        {
            in.settings.mib = "CPQPOWER-MIB::ups";
            in.settings.community = "cpqpqwer.114";
        }*/

        /*SECTION("Lenovo device lenovo.181")
        {
            in.settings.mib = "joint-iso-ccitt";
            in.settings.community = "lenovo.181";
        }*/

        SECTION("Genapi device xups.238")
        {
            in.settings.mib       = "EATON-OIDS::xupsMIB";
            in.settings.community = "xups.238";
        }

        SECTION("Genapi device xups.159")
        {
            in.settings.mib       = "EATON-OIDS::xupsMIB";
            in.settings.community = "xups.159";
        }

        msg.userData.setString(*pack::json::serialize(in));
        fty::Expected<fty::disco::Message> ret = Test::send(msg);
        if (!ret) {
            FAIL(ret.error());
        }

        proc.interrupt();
        proc.wait();
    } else {
        FAIL(pid.error());
    }
}

/*TEST_CASE("Assets / Powercom")
{
    fty::Message msg = Test::createMessage(fty::commands::assets::Subject);

    fty::commands::assets::In in;
    in.address = "10.130.33.34";
    in.protocol  = "nut_powercom";
    in.settings.timeout = 10000;
    in.settings.credentialId = "basic";

    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::Message> ret = Test::send(msg);
    if (!ret) {
        FAIL(ret.error());
    }
}*/
