#include "test-common.h"
#include "../server/src/jobs/impl/address.h"

TEST_CASE("Address / iteration", "[address]")
{
    address::AddressParser addr;

    // empty address
    CHECK(!addr.itIpInit("", ""));

    // without stop address
    auto res0 = addr.itIpInit("192.168.0.0", "");
    //std::cout << "res0=" << *res0 << std::endl;
    CHECK(*res0 == "192.168.0.0");
    CHECK(addr.itIpNext()->empty());

    // with stop address < start address
    auto res1 = addr.itIpInit("192.168.0.2", "192.168.0.1");
    CHECK(!res1);
    CHECK(!addr.itIpNext());

    // continuous address
    auto res2 = addr.itIpInit("192.168.0.0", "192.168.0.14");
    //std::cout << "res2=" << *res2 << std::endl;
    CHECK(*res2 == "192.168.0.0");
    int count = 1;
    auto previous = *res2;
    while (1) {
        auto res3 = addr.itIpNext();
        if (!res3) {
            FAIL("ERROR");
        }
        else if ((*res3).empty()) {
            break;
        }
        //std::cout << "res3=" << *res3 << std::endl;
        CHECK(previous != *res3);
        previous = *res3;
        count ++;
    }
    CHECK(count == 15);

    // discontinuous address
    auto res4 = addr.itIpInit("192.168.0.254", "192.168.1.14");
    //std::cout << "res4=" << *res4 << std::endl;
    CHECK(*res4 == "192.168.0.254");
    count = 1;
    previous = *res4;
    while (1) {
        auto res5 = addr.itIpNext();
        if (!res5) {
            FAIL("ERROR");
        }
        else if ((*res5).empty()) {
            break;
        }
        //std::cout << "res5=" << *res5 << std::endl;
        CHECK(previous != *res5);
        previous = *res5;
        count ++;
    }
    CHECK(count == 17);
}

TEST_CASE("Address / cidr", "[address]")
{
    // cidr test
    CHECK(!address::AddressParser::isCidr(""));
    CHECK(!address::AddressParser::isCidr("192.168.0.0"));
    CHECK(!address::AddressParser::isCidr("192.168.0.0-192.168.0.1"));
    CHECK(address::AddressParser::isCidr("192.168.0.0/24"));

    auto addrList0 = address::AddressParser::cidrToLimits("");
    CHECK(!addrList0);
    auto addrList1 = address::AddressParser::cidrToLimits("192.168.0.0/33");
    CHECK(!addrList1);
    auto addrList2 = address::AddressParser::cidrToLimits("192.168.0.0");
    CHECK(addrList2->first == "192.168.0.0");
    CHECK(addrList2->second == "192.168.0.0");
    auto addrList3 = address::AddressParser::cidrToLimits("192.168.0.0/24");
    CHECK(addrList3->first == "192.168.0.0");
    CHECK(addrList3->second == "192.168.0.255");
}

TEST_CASE("Address / range", "[address]")
{
    // range test
    CHECK(!address::AddressParser::isRange(""));
    CHECK(!address::AddressParser::isRange("192.168.0.0"));
    CHECK(!address::AddressParser::isRange("192.168.0.0/24"));
    CHECK(address::AddressParser::isRange("192.168.0.0-192.168.0.1"));

    auto rangeList0 = address::AddressParser::rangeToLimits("");
    CHECK(!rangeList0);
    auto rangeList1 = address::AddressParser::rangeToLimits("192.168.0.0");
    CHECK(!rangeList1);
    auto rangeList2 = address::AddressParser::rangeToLimits("-192.168.0.0");
    CHECK(!rangeList2);
    auto rangeList3 = address::AddressParser::rangeToLimits("192.168.0.0-192.168.0.255");
    CHECK(rangeList3->first == "192.168.0.0");
    CHECK(rangeList3->second == "192.168.0.255");
}

TEST_CASE("Address / getRangeIp", "[address]")
{
    auto addrList0 = address::AddressParser::getRangeIp("");
    CHECK(!addrList0);
    auto addrList1 = address::AddressParser::getRangeIp("192.168.0.0/33");
    CHECK(!addrList1);
    auto addrList2 = address::AddressParser::getRangeIp("192.168.0.0");
    CHECK(!addrList2);
    auto addrList3 = address::AddressParser::getRangeIp("192.168.0.0/24");
    CHECK((*addrList3).size() == 256);
    CHECK((*addrList3)[0] == "192.168.0.0");
    CHECK((*addrList3)[255] == "192.168.0.255");
    auto addrList4 = address::AddressParser::getRangeIp("192.168.0.0-192.168.0.255");
    CHECK((*addrList4).size() == 256);
    CHECK((*addrList4)[0] == "192.168.0.0");
    CHECK((*addrList4)[255] == "192.168.0.255");
}

TEST_CASE("Address / getLocalRangeIp", "[address]")
{
    //auto addr = address::AddressParser::getAddrCidr("", 0);
    //CHECK(!addr);
    auto addrList = address::AddressParser::getLocalRangeIp();
    CHECK(addrList);
    CHECK((*addrList).size() != 0);
}
