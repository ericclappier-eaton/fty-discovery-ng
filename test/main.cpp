#define CATCH_CONFIG_MAIN

#include "message-bus.h"
#include "message.h"
#include "src/config.h"
#include "src/discovery.h"
#include "src/jobs/protocols/snmp.h"
#include <catch2/catch.hpp>
#include <fty/fty-log.h>
#include <thread>

TEST_CASE("Discover")
{
    fty::Discovery dis("conf/discovery.conf");
    REQUIRE(dis.loadConfig());

    fty::protocol::Snmp::instance().init(fty::Config::instance().mibDatabase);
    fty::ManageFtyLog::setInstanceFtylog(fty::Config::instance().actorName, fty::Config::instance().logConfig);

    dis.init();
    std::thread th([&]() {
        dis.run();
    });

    SECTION("Send request")
    {
        using namespace std::chrono_literals;
        std::vector<std::thread> reqs;
        for (int i = 0; i < 1; ++i) {
            std::this_thread::sleep_for(10ms);
            reqs.emplace_back([&, num = i]() {
                fty::MessageBus bus;
                bus.init("unit-test" + std::to_string(num));
                fty::Message msg;
                msg.userData.append("10.130.38.125");
                msg.userData.append(std::to_string(num));

                msg.meta.to      = "discovery-ng-test";
                msg.meta.subject = "discovery";
                msg.meta.from    = "unit-test" + std::to_string(num);

                fty::Message ret = bus.send("discover", msg);
                logInfo() << "recieve: " << ret.dump();

                //                std::this_thread::sleep_for(100ms);

                fty::Message confmsg;
                confmsg.userData.append("10.130.38.125");
                confmsg.userData.append(std::to_string(num));

                confmsg.meta.to      = "discovery-ng-test";
                confmsg.meta.subject = "configure";
                confmsg.meta.from    = "unit-test" + std::to_string(num);

                fty::Message confret = bus.send("discover", confmsg);
                logInfo() << "recieve: " << confret.dump();

                //                fty::Message chmsg;
                //                chmsg.userData.append("10.130.38.125");
                //                chmsg.userData.append(std::to_string(num));

                //                chmsg.meta.to      = "discovery-ng-test";
                //                chmsg.meta.subject = "configure";
                //                chmsg.meta.from    = "unit-test" + std::to_string(num);

                //                fty::Message chret = bus.send("configure", chmsg);
                //                logInfo() << "recieve: " << chret.dump();
            });
        }
        std::cerr << "reqs done\n";
        for (auto& req : reqs) {
            req.join();
        }
    }

    dis.shutdown();
    th.join();
}
