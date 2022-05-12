#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_DISABLE_EXCEPTIONS

#include "test-common.h"
#include <catch2/catch.hpp>
#include <chrono>
#include <thread>

int main(int argc, char* argv[])
{
    using namespace std::chrono_literals;

    if (auto res = Test::init()) {
        int result = 0;
        result = Catch::Session().run(argc, argv);
        Test::shutdown();
        std::this_thread::sleep_for(200ms);
        return result;
    } else {
        std::cerr << "Cannot init test: " << res.error() << "\n\n";
        return 1;
    }
}
