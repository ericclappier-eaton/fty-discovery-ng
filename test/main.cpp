#define CATCH_CONFIG_MAIN
#include "discovery.h"
#include "message-bus.h"
#include "message.h"
#include <catch2/catch.hpp>
#include <fty/fty-log.h>
#include <thread>

TEST_CASE("Discover")
{
    fty::Discovery dis("test/conf/discovery.conf");
    REQUIRE(dis.loadConfig());

    fty::ManageFtyLog::setInstanceFtylog(dis.config().actorName, dis.config().logConfig);

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
                bus.init("unit-test"+std::to_string(num));
                fty::Message msg;
                msg.userData.append("10.130.38.125");//append("clement.roz.lab.etn.com");//append("127.0.0.1");
                msg.userData.append(std::to_string(num));

                msg.meta.to      = "discovery-ng-test";
                msg.meta.subject = "discovery";
                msg.meta.from    = "unit-test"+std::to_string(num);

                fty::Message ret = bus.send("discover", msg);
                logInfo() << "recieve: " << ret.dump();
            });
        }
        std::cerr << "reqs done\n";
        for(auto& req: reqs) {
            req.join();
        }
    }

    dis.shutdown();
    th.join();
}
