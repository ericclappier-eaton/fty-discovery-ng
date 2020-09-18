#define CATCH_CONFIG_RUNNER

#include "test-common.h"
#include <catch2/catch.hpp>

int main(int argc, char* argv[])
{
    if (auto res = Test::init()) {
        int result = Catch::Session().run(argc, argv);
        Test::shutdown();
        return result;
    } else {
        std::cerr << "Canot init test: " << res.error() << "\n\n";
        return 1;
    }
}
