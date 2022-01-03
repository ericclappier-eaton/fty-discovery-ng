#include "test-common.h"
#include "commands.h"
#include "../server/src/jobs/create-asset.h"
#include "../server/src/jobs/auto-discovery.h"
#include <fty/process.h>
#include <fty_common_socket_sync_client.h>
#include <fty_security_wallet.h>
#include <secw_producer_accessor.h>
#include <secw_security_wallet_server.h>
#include <fty_common_messagebus_message.h>
#include <fty_common_socket.h>
#include <fty_common_mlm.h>
#include <vector>
#include <utility>
#include <chrono>

using namespace fty::disco;

TEST_CASE("Auto disco / updateExt", "[auto]")
{
    commands::assets::Ext in;
    job::asset::create::Ext out;
    REQUIRE_NOTHROW(job::AutoDiscovery::updateExt(in, out));
    CHECK(out.size() == 0);
    pack::StringMap& it_in = in.append();
    it_in.append("key1",      "value1");
    it_in.append("read_only", "true");
    job::AutoDiscovery::updateExt(in, out);
    CHECK(out.size() == 1);
    job::asset::create::MapExtObj it_out;
    REQUIRE_THROWS(it_out  = out["key2"]);
    REQUIRE_NOTHROW(it_out = out["key1"]);
    CHECK(it_out.value    == "value1");
    CHECK(it_out.readOnly == true);
    CHECK(it_out.update   == true);
}

TEST_CASE("Auto disco / updateHostName", "[auto]")
{
    std::string address = "";
    job::asset::create::Ext ext;
    CHECK(!job::AutoDiscovery::updateHostName(address, ext));
    address = "no_host_name";
    CHECK(!job::AutoDiscovery::updateHostName(address, ext));
    address = "127.0.0.1";
    CHECK(job::AutoDiscovery::updateHostName(address, ext));
    // get hostname of the machine
    char hostname[HOST_NAME_MAX];
    auto result = gethostname(hostname, HOST_NAME_MAX);
    CHECK(!result);
    REQUIRE_NOTHROW(ext["hostname"]);
    CHECK(ext["hostname"].value    == hostname);
    CHECK(ext["hostname"].readOnly == false);
}

TEST_CASE("Auto disco / isDeviceCentricView", "[auto]")
{
    auto& discoAuto = Test::instance().getDisco().getAutoDiscovery();
    REQUIRE_NOTHROW(discoAuto.isDeviceCentricView());
}

TEST_CASE("Auto disco / readConfig", "[auto]")
{
    auto& discoAuto = Test::instance().getDisco().getAutoDiscovery();
    REQUIRE_NOTHROW(discoAuto.isDeviceCentricView());
}

TEST_CASE("Auto disco / status discovery update", "[auto]")
{
    auto& discoAuto = Test::instance().getDisco().getAutoDiscovery();
    discoAuto.initListIpAddressNb(4);
    discoAuto.initListIpAddressCount(4);
    discoAuto.statusDiscoveryReset();
    auto status = discoAuto.getStatus();
    CHECK(status.ups        == 0);
    CHECK(status.epdu       == 0);
    CHECK(status.sts        == 0);
    CHECK(status.sensors    == 0);
    CHECK(status.discovered == 0);
    CHECK(status.progress   == 0);
    discoAuto.updateStatusDiscoveryCounters("ups");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.ups        == 1);
    CHECK(status.epdu       == 0);
    CHECK(status.sts        == 0);
    CHECK(status.sensors    == 0);
    CHECK(status.discovered == 1);
    CHECK(status.progress   == 25);
    discoAuto.updateStatusDiscoveryCounters("epdu");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.ups        == 1);
    CHECK(status.epdu       == 1);
    CHECK(status.sts        == 0);
    CHECK(status.sensors    == 0);
    CHECK(status.discovered == 2);
    CHECK(status.progress   == 50);
    discoAuto.updateStatusDiscoveryCounters("sts");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.ups        == 1);
    CHECK(status.epdu       == 1);
    CHECK(status.sts        == 1);
    CHECK(status.sensors    == 0);
    CHECK(status.discovered == 3);
    CHECK(status.progress   == 75);
    discoAuto.updateStatusDiscoveryCounters("sensor");
    // caution normally no progress for sensor, just for test
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.ups        == 1);
    CHECK(status.epdu       == 1);
    CHECK(status.sts        == 1);
    CHECK(status.sensors    == 1);
    CHECK(status.discovered == 4);
    CHECK(status.progress   == 100);
    // test limits
    discoAuto.updateStatusDiscoveryCounters("ups");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.ups        == 2);
    CHECK(status.epdu       == 1);
    CHECK(status.sts        == 1);
    CHECK(status.sensors    == 1);
    CHECK(status.discovered == 5);
    CHECK(status.progress   == 100);
    discoAuto.updateStatusDiscoveryCounters("bad_type");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.ups        == 2);
    CHECK(status.epdu       == 1);
    CHECK(status.sts        == 1);
    CHECK(status.sensors    == 1);
    CHECK(status.discovered == 5);
    CHECK(status.progress   == 100);
}

TEST_CASE("Auto disco / Empty request", "[auto]")
{
    fty::disco::Message                msg = Test::createMessage(fty::disco::commands::scan::start::Subject);
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: payload is empty" == ret.error());
}

TEST_CASE("Auto disco / Wrong request", "[auto]")
{
    fty::disco::Message msg = Test::createMessage(fty::disco::commands::scan::start::Subject);

    msg.userData.setString("Some shit");
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    CHECK_FALSE(ret);
    CHECK("Wrong input data: format of payload is incorrect" == ret.error());
}

using namespace fty::disco::commands::scan;

// get discovery status
auto getStatus = []() -> const status::Out {
    status::Out res;

    fty::disco::Message msgStatus = Test::createMessage(status::Subject);
    fty::Expected<fty::disco::Message> ret = Test::send(msgStatus);
    if (!ret) {
        FAIL(ret.error());
    }
    std::cout << "Status:" << ret->userData.asString() << std::endl;
    auto info = pack::json::deserialize(ret->userData.asString(), res);
    if (!info) {
        FAIL(info.error());
    }
    return res;
};

TEST_CASE("Auto disco / Test normal scan auto", "[auto]")
{
    auto& discoAuto = Test::instance().getDisco().getAutoDiscovery();
    discoAuto.statusDiscoveryInit();

    // Test status before scan
    auto out = getStatus();
    CHECK(out.status     == status::Out::Status::Unknown);
    CHECK(out.discovered == 0);
    CHECK(out.ups        == 0);
    CHECK(out.epdu       == 0);
    CHECK(out.sts        == 0);
    CHECK(out.sensors    == 0);

    // Prepare discovery
    fty::disco::Message msg = Test::createMessage(start::Subject);
    start::In in;
    in.type = start::In::Type::Ip;
    for (int i = 0; i < 100; i++) {
        in.ips.append("127.0.0.1");
    }
    in.protocols.append("nut_snmp:1161");
    in.documents.append("no_id");

    // Execute discovery
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    if (!ret) {
        FAIL(ret.error());
    }

    // Check status (in progress)
    out = getStatus();
    CHECK(out.status     == status::Out::Status::InProgress);
    //CHECK(out.progress == "0%");
    CHECK(out.discovered == 0);
    CHECK(out.ups        == 0);
    CHECK(out.epdu       == 0);
    CHECK(out.sts        == 0);
    CHECK(out.sensors    == 0);

    auto start = std::chrono::steady_clock::now();
    while(1) {
        out = getStatus();
        if (out.status == status::Out::Status::Terminated) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto end = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 10) {
            FAIL("Timeout when wait terminated status");
        }
    }

    // Check status (terminated)
    out = getStatus();
    CHECK(out.status == status::Out::Status::Terminated);
    CHECK(out.progress == "100%");
}

TEST_CASE("Auto disco / Test stop scan auto", "[auto]")
{
    auto& discoAuto = Test::instance().getDisco().getAutoDiscovery();
    discoAuto.statusDiscoveryInit();

    // Test status before scan
    auto out = getStatus();
    CHECK(out.status     == status::Out::Status::Unknown);
    CHECK(out.discovered == 0);
    CHECK(out.ups        == 0);
    CHECK(out.epdu       == 0);
    CHECK(out.sts        == 0);
    CHECK(out.sensors    == 0);

    // Prepare discovery
    fty::disco::Message msg = Test::createMessage(start::Subject);
    start::In in;
    in.type = start::In::Type::Ip;
    for (int i = 0; i < 100; i++) {
        in.ips.append("127.0.0.1");
    }
    in.protocols.append("nut_snmp:1161");
    in.documents.append("no_id");

    // Execute discovery
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    if (!ret) {
        FAIL(ret.error());
    }

    // Check status (in progress)
    out = getStatus();
    CHECK(out.status == status::Out::Status::InProgress);
    //CHECK(out.progress == "0%");
    CHECK(out.discovered == 0);
    CHECK(out.ups        == 0);
    CHECK(out.epdu       == 0);
    CHECK(out.sts        == 0);
    CHECK(out.sensors    == 0);

    // Stop discovery
    fty::disco::Message msg2 = Test::createMessage(stop::Subject);
    fty::Expected<fty::disco::Message> ret2 = Test::send(msg2);
    if (!ret2) {
        FAIL(ret2.error());
    }

    auto start = std::chrono::steady_clock::now();
    while(1) {
        out = getStatus();
        if (out.status == status::Out::Status::Terminated ||
            out.status == status::Out::Status::CancelledByUser) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto end = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 10) {
            FAIL("Timeout when wait terminated status");
        }
    }

    // Check status (terminated)
    out = getStatus();
    CHECK(out.status     == status::Out::Status::CancelledByUser);
    CHECK(!(out.progress == "100%"));  // normally not finished
}

class TestAuto {
public:
    TestAuto(fty::disco::MessageBus& bus) : m_bus(bus) {};
    void recAssets(const fty::disco::Message& msg);
private:
    fty::disco::MessageBus& m_bus;
};

void TestAuto::recAssets(const fty::disco::Message& msg)
{
    messagebus::Message msg2 = msg.toMessageBus();
    if (msg2.metaData()[messagebus::Message::SUBJECT] == "CREATE") {
        logDebug("RECEIVE CREATE MSG OK");
        messagebus::Message answ;
        answ.metaData().emplace(messagebus::Message::SUBJECT, msg2.metaData().find(messagebus::Message::SUBJECT)->second);
        answ.metaData().emplace(messagebus::Message::FROM, "asset-agent-ng");
        answ.metaData().emplace(messagebus::Message::TO, msg2.metaData().find(messagebus::Message::FROM)->second);
        answ.metaData().emplace(messagebus::Message::CORRELATION_ID, msg2.metaData().find(messagebus::Message::CORRELATION_ID)->second);
        answ.metaData().emplace(messagebus::Message::STATUS, messagebus::STATUS_OK);
        answ.userData() = msg2.userData();

        // send response
        m_bus.send(msg2.metaData().find(messagebus::Message::REPLY_TO)->second, Message(answ));
    }
}

TEST_CASE("Auto disco / Test real scan auto with simulation", "[auto]")
{
    // clang-format off
    fty::Process proc("snmpsimd",  {
        "--data-dir=assets",
        "--agent-udpv4-endpoint=127.0.0.1:1161",
        "--logging-method=file:.snmpsim.txt",
        "--variation-modules-dir=assets"
    });
    // clang-format on
    if (auto pid = proc.run(); !pid) {
        FAIL(pid.error());
    }

    static const char* endpoint_disco = "inproc://fty-discovery-ng-test";

    static constexpr const char* Name = "asset-agent-ng";
    static constexpr const char* Subject = "CREATE";
    static constexpr const char* Queue   = "FTY.Q.ASSET.QUERY";

    // Create message bus for asset creation
    fty::disco::MessageBus bus;
    if (auto res = bus.init("asset-agent-ng", endpoint_disco); !res) {
        FAIL("Bus asset init");
    }
    TestAuto testAuto(bus);
    auto sub = bus.subsribe("FTY.Q.ASSET.QUERY", &TestAuto::recAssets, &testAuto);

    // Create a stream publisher for security wallet notification
    mlm::MlmStreamClient notificationStream(SECURITY_WALLET_AGENT, SECW_NOTIFICATIONS, 1000, endpoint_disco);

    // Create the server for security wallet
    secw::SecurityWalletServer serverSecw("conf/configuration.json", "conf/data.json", notificationStream);
    fty::SocketBasicServer     agentSecw(serverSecw, "secw-test.socket");
    std::thread                agentSecwThread(&fty::SocketBasicServer::run, &agentSecw);

    // Create client for security wallet
    fty::SocketSyncClient  secwSyncClient("secw-test.socket");
    secw::ProducerAccessor producerAccessor(secwSyncClient);

    auto& discoAuto = Test::instance().getDisco().getAutoDiscovery();
    discoAuto.statusDiscoveryInit();

    auto initStatus = [](status::Out::Status status,
            int discovered, int ups, int epdu, int sts, int sensors) -> const status::Out {
        status::Out out;
        out.status     = status;
        out.discovered = discovered;
        out.ups        = ups;
        out.epdu       = epdu;
        out.sts        = sts;
        out.sensors    = sensors;
        return out;
    };
    int i = 0;
    //for (const auto& testCases : std::vector<std::pair<std::vector<std::string>, const status::Out>>{
    for (const auto& testCases : std::vector<std::pair<std::string, const status::Out>>{
        // Daisy device epdu.147
        { "epdu.147", initStatus(status::Out::Status::Terminated, 4, 0, 4, 0, 0
        )},
        // MG device mge.125
        { "mge.125",  initStatus(status::Out::Status::Terminated, 1, 1, 0, 0, 0
        )},
        // MG device mge.191
        { "mge.191",  initStatus(status::Out::Status::Terminated, 1, 1, 0, 0, 0
        )},
        // Genepi device xups.238
        { "xups.238", initStatus(status::Out::Status::Terminated, 1, 1, 0, 0, 0
        )},
        // Genepi device xups.159
        { "xups.159", initStatus(status::Out::Status::Terminated, 2, 1, 0, 0, 1
        )},
        // Ats device ats.100
        { "ats.100",  initStatus(status::Out::Status::Terminated, 1, 0, 0, 1, 0
        )},
        // Genepi device xups.238 & Genepi device xups.159
        /*{ {"xups.238", "xups.159"}, initStatus(status::Out::Status::Terminated, 3, 2, 0, 0, 1
        )},*/
    }) {
        std::cout << "TEST #" << i << std::endl;
        //auto passwordList = testCases.first;
        auto password = testCases.first;
        auto statusExpected = testCases.second;
        // create passwords in security wallet
        std::vector<std::string> idList;
        //for (const auto password : passwordList) {
            bool isFound = true;
            try {
                auto doc = producerAccessor.getDocumentWithoutPrivateDataByName("default", password);
                if (!doc) {
                    isFound = false;
                }
                else {
                    idList.push_back(doc->getId());
                }
            }
            catch(secw::SecwNameDoesNotExistException& ex) {
                isFound = false;
            }
            // create password if not exist
            if (!isFound) {
                secw::Snmpv1Ptr doc = std::make_shared<secw::Snmpv1>("community", password);
                doc->setName(password);
                doc->addUsage("discovery_monitoring");
                idList.push_back(producerAccessor.insertNewDocument("default", std::dynamic_pointer_cast<secw::Document>(doc)));
            }
        //}
        // Test status before scan
        auto out = getStatus();
        if (i == 0) {
            //CHECK(out.status   == status::Out::Status::Unknown); // TBD ???
            CHECK(out.discovered == 0);
            CHECK(out.ups        == 0);
            CHECK(out.epdu       == 0);
            CHECK(out.sts        == 0);
            CHECK(out.sensors    == 0);
        }

        // Prepare discovery
        fty::disco::Message msg = Test::createMessage(start::Subject);
        start::In in;
        in.type = start::In::Type::Ip;
        // TBD Use 169.254.50.X private address for multi scan
        // ip address add dev eth0 scope link $addr/16
        // ip address del $addr/16 dev eth0
        in.ips.append("127.0.0.1");
        in.protocols.append("nut_snmp:1161");
        for (const auto id : idList) {
            in.documents.append(id);
        }

        // Execute discovery
        msg.userData.setString(*pack::json::serialize(in));
        fty::Expected<fty::disco::Message> ret = Test::send(msg);
        if (!ret) {
            FAIL(ret.error());
        }

        // Wait start of discovery
        auto start = std::chrono::steady_clock::now();
        while(1) {
            out = getStatus();
            if (out.status == status::Out::Status::InProgress) {
                CHECK(out.progress   == "0%");
                CHECK(out.discovered == 0);
                CHECK(out.ups        == 0);
                CHECK(out.epdu       == 0);
                CHECK(out.sts        == 0);
                CHECK(out.sensors    == 0);
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // Timeout of 20 sec
            auto end = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 20) {
                FAIL("Timeout when wait progress status");
            }
        }

        // Wait end of discovery
        start = std::chrono::steady_clock::now();
        while(1) {
            out = getStatus();
            if (out.status == status::Out::Status::Terminated) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // Timeout of 60 sec
            auto end = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 60) {
                FAIL("Timeout when wait terminated status");
            }
        }

        // Check status (terminated)
        out = getStatus();
        CHECK(out.status     == status::Out::Status::Terminated);
        CHECK(out.progress   == "100%");
        CHECK(out.discovered == statusExpected.discovered);
        CHECK(out.ups        == statusExpected.ups);
        CHECK(out.epdu       == statusExpected.epdu);
        CHECK(out.sts        == statusExpected.sts);
        CHECK(out.sensors    == statusExpected.sensors);

        // delete passwords in security wallet
        for (const auto id : idList) {
            producerAccessor.deleteDocument("default", id);
        }
        i ++;
    }
    agentSecw.requestStop();
    agentSecwThread.join();

    proc.interrupt();
    proc.wait();
}
