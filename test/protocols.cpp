#include "test-common.h"

namespace fty::disco {

TEST_CASE("Protocols/ Empty request")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: payload is empty" == ret.error());
}

TEST_CASE("Protocols / Wrong request")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    msg.userData.setString("Some shit");
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: format of payload is incorrect" == ret.error());
}

TEST_CASE("Protocols / Unaviable host")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    commands::protocols::In in;
    in.address = "pointtosky.roz.lab.etn.com";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available: pointtosky.roz.lab.etn.com" == ret.error());
}

TEST_CASE("Protocols / Not asset")
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

TEST_CASE("Protocols / Invalid ip")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    commands::protocols::In in;
    in.address = "127.0.145.256";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available: 127.0.145.256" == ret.error());
}

TEST_CASE("Protocols / Fake request")
{
    fty::disco::Message msg = Test::createMessage(commands::protocols::Subject);

    commands::protocols::In in;
    in.address = "__fake__";
    msg.userData.setString(*pack::json::serialize(in));

    fty::Expected<fty::disco::Message> ret = Test::send(msg);

    CHECK(ret);
    auto res = ret->userData.decode<commands::protocols::Out>();
    CHECK(res);
    CHECK(2 == res->size());
    CHECK("nut_snmp" == (*res)[0]);
    CHECK("nut_xml_pdc" == (*res)[1]);
}

TEST_CASE("Protocols / resolve")
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
