#include "test-common.h"
#include "commands.h"
#include "discovery-config-manager.h"
#include "../server/src/jobs/create-asset.h"
#include "../server/src/jobs/auto-discovery.h"
#include "../server/src/jobs/impl/credentials.h"
#include <fty/process.h>
#include <fty_security_wallet.h>
#include <fty_common_messagebus_message.h>
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
    CHECK(!out.contains("key2"));
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
    // TBD: Doesn't work on Jenkins
    //CHECK(ext["hostname"].value  == hostname);
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
    discoAuto.statusDiscoveryReset(4);
    auto status = discoAuto.getStatus();
    CHECK(status.numOfAddress   == 4);
    CHECK(status.addressScanned == 0);
    status = discoAuto.getStatus();
    CHECK(status.addressScanned == 0);
    CHECK(status.discovered     == 0);
    CHECK(status.ups            == 0);
    CHECK(status.epdu           == 0);
    CHECK(status.sts            == 0);
    CHECK(status.sensors        == 0);
    discoAuto.updateStatusDiscoveryCounters("ups");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.addressScanned == 1);
    CHECK(status.discovered     == 1);
    CHECK(status.ups            == 1);
    CHECK(status.epdu           == 0);
    CHECK(status.sts            == 0);
    CHECK(status.sensors        == 0);
    discoAuto.updateStatusDiscoveryCounters("epdu");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.addressScanned == 2);
    CHECK(status.discovered     == 2);
    CHECK(status.ups            == 1);
    CHECK(status.epdu           == 1);
    CHECK(status.sts            == 0);
    CHECK(status.sensors        == 0);
    discoAuto.updateStatusDiscoveryCounters("sts");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.addressScanned == 3);
    CHECK(status.discovered     == 3);
    CHECK(status.ups            == 1);
    CHECK(status.epdu           == 1);
    CHECK(status.sts            == 1);
    CHECK(status.sensors        == 0);
    discoAuto.updateStatusDiscoveryCounters("sensor");
    // caution normally no progress for sensor, just for test
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    CHECK(status.addressScanned == 4);
    CHECK(status.discovered     == 4);
    CHECK(status.ups            == 1);
    CHECK(status.epdu           == 1);
    CHECK(status.sts            == 1);
    CHECK(status.sensors        == 1);
    // test limits
    discoAuto.updateStatusDiscoveryCounters("ups");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
    // Note: addressScanned cannot be superior to total address
    CHECK(status.addressScanned == 4);
    CHECK(status.discovered     == 5);
    CHECK(status.ups            == 2);
    CHECK(status.epdu           == 1);
    CHECK(status.sts            == 1);
    CHECK(status.sensors        == 1);
    discoAuto.updateStatusDiscoveryCounters("bad_type");
    discoAuto.updateStatusDiscoveryProgress();
    status = discoAuto.getStatus();
   // Note: Cannot add bad type of device
    CHECK(status.addressScanned == 4);
    CHECK(status.discovered     == 5);
    CHECK(status.ups            == 2);
    CHECK(status.epdu           == 1);
    CHECK(status.sts            == 1);
    CHECK(status.sensors        == 1);
}

TEST_CASE("Auto disco / Empty request", "[auto]")
{
    auto msg = Test::createMessage(fty::disco::commands::scan::start::Subject);
    auto ret = Test::send(msg);
    CHECK_FALSE(ret);
}

using namespace fty::disco::commands::scan;

// get discovery status
auto getStatus = []() -> const status::Out {
    status::Out res;

    auto msgStatus = Test::createMessage(status::Subject);
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
    CHECK(out.status         == status::Out::Status::Unknown);
    CHECK(out.numOfAddress   == 0);
    CHECK(out.addressScanned == 0);
    CHECK(out.discovered     == 0);
    CHECK(out.ups            == 0);
    CHECK(out.epdu           == 0);
    CHECK(out.sts            == 0);
    CHECK(out.sensors        == 0);

    // clang-format off
    fty::Process proc("snmpsimd", {
        "--data-dir=root",
        "--agent-udpv4-endpoint=127.0.0.1:1161",
        "--logging-method=file:.snmpsim.txt",
        "--variation-modules-dir=root",
    });
    // clang-format on

    if (auto pid = proc.run(); !pid) {
        FAIL(pid.error());
    }

    // Wait a moment for snmpsim init
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Prepare discovery
    const int nbAddressToTest = 2000;
    start::In in;
    in.discovery.type = ConfigDiscovery::Discovery::Type::Ip;
    for (int i = 0; i < nbAddressToTest; i++) {
        in.discovery.ips.append("127.0.0.1");
    }
    ConfigDiscovery::Protocol nutSnmp;
    nutSnmp.protocol = ConfigDiscovery::Protocol::Type::Snmp;
    nutSnmp.ports.append(1161);
    in.discovery.protocols.append(nutSnmp);
    in.discovery.documents.append("no_id");

    // Execute discovery
    fty::disco::Message msg = Test::createMessage(start::Subject);
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    if (!ret) {
        FAIL(ret.error());
    }
    logDebug("Check status in progress");
    // Check status (in progress)
    out = getStatus();
    CHECK(out.status     == status::Out::Status::InProgess);
    CHECK(out.discovered == 0);
    CHECK(out.ups        == 0);
    CHECK(out.epdu       == 0);
    CHECK(out.sts        == 0);
    CHECK(out.sensors    == 0);

    auto start = std::chrono::steady_clock::now();
    while(1) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto end = std::chrono::steady_clock::now();
        logDebug("Check status terminated");
        out = getStatus();
        if (out.status == status::Out::Status::Terminated) {
            logDebug("Check status terminated detected after: {} sec",
                std::chrono::duration_cast<std::chrono::seconds>(end - start).count());
            break;
        }
        if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 20) {
            FAIL("Timeout when wait terminated status");
        }
    }

    // Check status (terminated)
    out = getStatus();
    CHECK(out.status == status::Out::Status::Terminated);
    CHECK(out.addressScanned == nbAddressToTest);

    // Stop snmp process
    proc.interrupt();
    proc.wait();
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

    // clang-format off
    fty::Process proc("snmpsimd", {
        "--data-dir=root",
        "--agent-udpv4-endpoint=127.0.0.1:1161",
        "--logging-method=file:.snmpsim.txt",
        "--variation-modules-dir=root",
    });
    // clang-format on

    if (auto pid = proc.run(); !pid) {
        FAIL(pid.error());
    }

    // Wait a moment for snmpsim init
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Prepare discovery
    const int nbAddressToTest = 1000;
    start::In in;
    in.discovery.type = ConfigDiscovery::Discovery::Type::Ip;
    for (int i = 0; i < nbAddressToTest; i++) {
        in.discovery.ips.append("127.0.0.1");
    }
    ConfigDiscovery::Protocol nutSnmp;
    nutSnmp.protocol = ConfigDiscovery::Protocol::Type::Snmp;
    nutSnmp.ports.append(1161);
    in.discovery.protocols.append(nutSnmp);
    in.discovery.documents.append("no_id");

    // Execute discovery
    fty::disco::Message msg = Test::createMessage(start::Subject);
    msg.userData.setString(*pack::json::serialize(in));
    fty::Expected<fty::disco::Message> ret = Test::send(msg);
    if (!ret) {
        FAIL(ret.error());
    }

    // Check status (in progress)
    logDebug("Check status in progress");
    out = getStatus();
    CHECK(out.status == status::Out::Status::InProgess);
    //CHECK(out.addressScanned == 0);
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
        auto end = std::chrono::steady_clock::now();
        logDebug("Check status terminated or cancelled");
        out = getStatus();
        if (out.status == status::Out::Status::Terminated || out.status == status::Out::Status::CancelledByUser) {
            logDebug("Check status terminated or cancelled detected after: {} sec",
                std::chrono::duration_cast<std::chrono::seconds>(end - start).count());
            break;
        }
        if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 20) {
            FAIL("Timeout when wait terminated or cancelled status");
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Check status (terminated)
    out = getStatus();
    CHECK(out.status == status::Out::Status::CancelledByUser);
    //CHECK(!(out.addressScanned == nbAddressToTest));  // normally not finished

    // Stop snmp process
    proc.interrupt();
    proc.wait();
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
        logDebug("Receive create asset message");
        messagebus::Message answ;
        answ.metaData().emplace(messagebus::Message::SUBJECT, msg2.metaData().find(messagebus::Message::SUBJECT)->second);
        answ.metaData().emplace(messagebus::Message::FROM, "asset-agent-ng");
        answ.metaData().emplace(messagebus::Message::TO, msg2.metaData().find(messagebus::Message::FROM)->second);
        answ.metaData().emplace(messagebus::Message::CORRELATION_ID, msg2.metaData().find(messagebus::Message::CORRELATION_ID)->second);
        answ.metaData().emplace(messagebus::Message::STATUS, messagebus::STATUS_OK);
        answ.userData() = msg2.userData();

        // send response
        auto res = m_bus.send(msg2.metaData().find(messagebus::Message::REPLY_TO)->second, Message(answ));
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

    // Wait a moment for snmpsim init
    std::this_thread::sleep_for(std::chrono::seconds(1));

    static const char* endpoint_disco = "inproc://fty-discovery-ng-test";

    // Create message bus for asset creation
    fty::disco::MessageBus bus;
    if (auto res = bus.init("asset-agent-ng", endpoint_disco); !res) {
        FAIL("Bus asset init");
    }
    TestAuto testAuto(bus);
    auto sub = bus.subsribe("FTY.Q.ASSET.QUERY", &TestAuto::recAssets, &testAuto);

    auto& discoAuto = Test::instance().getDisco().getAutoDiscovery();
    discoAuto.statusDiscoveryInit();

    auto initStatus = [](status::Out::Status status,
        uint32_t discovered, uint32_t ups, uint32_t epdu, uint32_t sts, uint32_t sensors) -> const status::Out {
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
        std::cout << "TEST #" << i ++ << std::endl;
        //auto passwordList = testCases.first;
        auto password = testCases.first;
        auto statusExpected = testCases.second;

        // set credential callback
        fty::disco::impl::setCredentialsService([password](const std::string&) -> secw::DocumentPtr {
            secw::Snmpv1Ptr doc = std::make_shared<secw::Snmpv1>("community", password);
            doc->setName(password);
            doc->addUsage("discovery_monitoring");
            return doc;
        });

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
        start::In in;
        in.discovery.type = ConfigDiscovery::Discovery::Type::Ip;
        // TBD Use 169.254.50.X private address for multi scan
        // ip address add dev eth0 scope link $addr/16
        // ip address del $addr/16 dev eth0
        in.discovery.ips.append("127.0.0.1");
        ConfigDiscovery::Protocol nutSnmp;
        nutSnmp.protocol = ConfigDiscovery::Protocol::Type::Snmp;
        nutSnmp.ports.append(1161);
        in.discovery.protocols.append(nutSnmp);
        // note: document id is equal to the password to simplify the test
        in.discovery.documents.append(password);

        // Execute discovery
        fty::disco::Message msg = Test::createMessage(start::Subject);
        msg.userData.setString(*pack::json::serialize(in));
        fty::Expected<fty::disco::Message> ret = Test::send(msg);
        if (!ret) {
            FAIL(ret.error());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Wait start of discovery
        auto start = std::chrono::steady_clock::now();
        while(1) {
            auto end = std::chrono::steady_clock::now();
            out = getStatus();
            if (out.status == status::Out::Status::InProgess) {
                logDebug("Check status in progress detected after: {} sec",
                    std::chrono::duration_cast<std::chrono::seconds>(end - start).count());
                CHECK(out.addressScanned == 0);
                CHECK(out.discovered     == 0);
                CHECK(out.ups            == 0);
                CHECK(out.epdu           == 0);
                CHECK(out.sts            == 0);
                CHECK(out.sensors        == 0);
                break;
            }
            // Timeout of 100 sec
            if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 100) {
                FAIL("Timeout when wait progress status");
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Wait end of discovery
        // TBD: Need to rework this test to reduce timeout
        start = std::chrono::steady_clock::now();
        while(1) {
            auto end = std::chrono::steady_clock::now();
            logDebug("Check status terminated");
            out = getStatus();
            if (out.status == status::Out::Status::Terminated) {
                logDebug("Check status terminated detected after: {} sec",
                    std::chrono::duration_cast<std::chrono::seconds>(end - start).count());
                break;
            }
            // Timeout of 300 sec
            if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 300) {
                FAIL("Timeout when wait terminated status");
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        // Wait a little for creation of asset
        std::this_thread::sleep_for(std::chrono::seconds(15));

        // Check status (terminated)
        out = getStatus();
        CHECK(out.status         == status::Out::Status::Terminated);
        CHECK(out.addressScanned == 1);
        CHECK(out.discovered     == statusExpected.discovered);
        CHECK(out.ups            == statusExpected.ups);
        CHECK(out.epdu           == statusExpected.epdu);
        CHECK(out.sts            == statusExpected.sts);
        CHECK(out.sensors        == statusExpected.sensors);
    }

    proc.interrupt();
    proc.wait();
}
