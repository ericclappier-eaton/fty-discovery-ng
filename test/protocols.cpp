#include "test-common.h"

TEST_CASE("Protocols/ Empty request")
{
    fty::Message msg = Test::createMessage(fty::commands::protocols::Subject);

    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data" == ret.error());
}

TEST_CASE("Protocols / Wrong request")
{
    fty::Message msg = Test::createMessage(fty::commands::protocols::Subject);

    msg.userData.setString("Some shit");
    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data" == ret.error());
}

TEST_CASE("Protocols / Unaviable host")
{
    fty::Message msg = Test::createMessage(fty::commands::protocols::Subject);

    fty::commands::protocols::In in;
    in.address = "pointtosky";
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Host is not available" == ret.error());
}

TEST_CASE("Protocols / Fake request")
{
    fty::Message msg = Test::createMessage(fty::commands::protocols::Subject);

    fty::commands::protocols::In in;
    in.address = "__fake__";
    msg.userData.setString(*pack::json::serialize(in));

    fty::Expected<fty::Message> ret = Test::send(msg);

    CHECK(ret);
    auto res = ret->userData.decode<fty::commands::protocols::Out>();
    CHECK(res);
    CHECK(2 == res->size());
    CHECK("NUT_SNMP" == (*res)[0]);
    CHECK("NUT_XML_PDC" == (*res)[1]);
}
