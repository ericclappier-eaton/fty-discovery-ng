#define CATCH_CONFIG_MAIN

#include "commands.h"
#include "message-bus.h"
#include "message.h"
#include "src/config.h"
#include "src/discovery.h"
#include "src/jobs/protocols/snmp.h"
#include <catch2/catch.hpp>
#include <fty_log.h>
#include <thread>

struct DiscoverTest
{
    DiscoverTest()
        : m_dis("conf/discovery.conf")
    {
        REQUIRE(m_dis.loadConfig());

        fty::protocol::Snmp::instance().init(fty::Config::instance().mibDatabase);
        ManageFtyLog::setInstanceFtylog(fty::Config::instance().actorName, fty::Config::instance().logConfig);

        if (auto res = m_dis.init(); !res) {
            FAIL(res.error());
        }

        if (auto res = m_bus.init("unit-test"); !res) {
            FAIL(res.error());
        }

        m_th = std::thread([&]() {
            m_dis.run();
        });
    }

    ~DiscoverTest()
    {
        m_dis.shutdown();
        m_th.join();
    }

    fty::Message createProtocolMessage()
    {
        fty::Message msg;
        msg.meta.to      = fty::Config::instance().actorName;
        msg.meta.subject = fty::commands::protocols::Subject;
        msg.meta.from    = "unit-test";
        return msg;
    }

    fty::Message createMibsMessage()
    {
        fty::Message msg;
        msg.meta.to      = fty::Config::instance().actorName;
        msg.meta.subject = fty::commands::mibs::Subject;
        msg.meta.from    = "unit-test";
        return msg;
    }

    fty::MessageBus& bus()
    {
        return m_bus;
    }

private:
    fty::Discovery  m_dis;
    std::thread     m_th;
    fty::MessageBus m_bus;
};

TEST_CASE("Discover")
{
    DiscoverTest test;

    SECTION("Empty request")
    {
        fty::Message                msg = test.createProtocolMessage();
        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
        CHECK_FALSE(ret);
        CHECK("Wrong input data" == ret.error());
    }

    SECTION("Wrong request")
    {
        fty::Message msg = test.createProtocolMessage();
        msg.userData.setString("Some shit");
        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
        CHECK_FALSE(ret);
        CHECK("Wrong input data" == ret.error());
    }

    //    SECTION("Unaviable host")
    //    {
    //        fty::Message msg = test.createProtocolMessage();
    //        fty::commands::protocols::In in;
    //        in.address = "pointtosky";
    //        msg.userData.setString(*pack::json::serialize(in));
    //        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
    //        CHECK_FALSE(ret);
    //        CHECK("Host is not available" == ret.error());
    //    }

    SECTION("Fake request")
    {
        fty::Message                 msg = test.createProtocolMessage();
        fty::commands::protocols::In in;
        in.address = "__fake__";
        msg.userData.setString(*pack::json::serialize(in));
        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
        CHECK(ret);
        auto res = ret->userData.decode<fty::commands::protocols::Out>();
        CHECK(res);
        CHECK(2 == res->size());
        CHECK("NUT_SNMP" == (*res)[0]);
        CHECK("NUT_XML_PDC" == (*res)[1]);
    }

    SECTION("Real request")
    {
        fty::Message                 msg = test.createProtocolMessage();
        fty::commands::protocols::In in;
        in.address = "10.130.38.125";
        msg.userData.setString(*pack::json::serialize(in));
        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
        CHECK(ret);
        auto res = ret->userData.decode<fty::commands::protocols::Out>();
        CHECK(res);
        CHECK(2 == res->size());
        CHECK("NUT_SNMP" == (*res)[0]);
        CHECK("NUT_XML_PDC" == (*res)[1]);
    }
}

TEST_CASE("Mibs")
{
    DiscoverTest test;

    SECTION("Empty request")
    {
        fty::Message                msg = test.createMibsMessage();
        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
        CHECK_FALSE(ret);
        CHECK("Wrong input data" == ret.error());
    }

    SECTION("Wrong request")
    {
        fty::Message msg = test.createMibsMessage();
        msg.userData.setString("Some shit");
        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
        CHECK_FALSE(ret);
        CHECK("Wrong input data" == ret.error());
    }

    SECTION("Unaviable host")
    {
        fty::Message msg = test.createMibsMessage();
        fty::commands::protocols::In in;
        in.address = "pointtosky";
        msg.userData.setString(*pack::json::serialize(in));
        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
        CHECK_FALSE(ret);
        CHECK("Host is not available" == ret.error());
    }

    SECTION("Fake request")
    {
        fty::Message                 msg = test.createMibsMessage();
        fty::commands::protocols::In in;
        in.address = "__fake__";
        msg.userData.setString(*pack::json::serialize(in));
        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
        CHECK(ret);
        auto res = ret->userData.decode<fty::commands::protocols::Out>();
        CHECK(res);
        CHECK(3 == res->size());
        CHECK("MG-SNMP-UPS-MIB" == (*res)[0]);
        CHECK("UPS-MIB" == (*res)[1]);
        CHECK("XUPS-MIB" == (*res)[2]);
    }

    SECTION("Real request")
    {
        fty::Message                 msg = test.createMibsMessage();
        fty::commands::protocols::In in;
        in.address = "10.130.38.125";
        msg.userData.setString(*pack::json::serialize(in));
        fty::Expected<fty::Message> ret = test.bus().send(fty::Channel, msg);
        CHECK(ret);
        auto res = ret->userData.decode<fty::commands::protocols::Out>();
        CHECK(res);
        CHECK(3 == res->size());
        CHECK("XUPS-MIB" == (*res)[0]);
        CHECK("MG-SNMP-UPS-MIB" == (*res)[1]);
        CHECK("UPS-MIB" == (*res)[2]);
    }
}
