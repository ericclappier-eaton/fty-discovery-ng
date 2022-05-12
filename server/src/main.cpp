#include "config.h"
#include "daemon.h"
#include "discovery.h"
#include "jobs/impl/snmp.h"
#include <fty/command-line.h>
#include <fty_log.h>

int main(int argc, char** argv)
{
    bool        daemon = false;
    std::string config = "conf/discovery.conf";
    bool        help   = false;

    // clang-format off
    fty::CommandLine cmd("New discovery service", {
        {"--config", config, "Configuration file"},
        {"--daemon", daemon, "Daemonize this application"},
        {"--help",   help,   "Show this help"}
    });
    // clang-format on

    if (auto res = cmd.parse(argc, argv); !res) {
        std::cerr << res.error() << std::endl;
        std::cout << std::endl;
        std::cout << cmd.help() << std::endl;
        return EXIT_FAILURE;
    }

    if (help) {
        std::cout << cmd.help() << std::endl;
        return EXIT_SUCCESS;
    }

    fty::disco::Discovery dis(config);
    if (!dis.loadConfig()) {
        return EXIT_FAILURE;
    }

    fty::disco::impl::Snmp::instance().init(fty::disco::Config::instance().mibDatabase.value());
    ManageFtyLog::setInstanceFtylog(fty::disco::Config::instance().actorName.value(),
                                    fty::disco::Config::instance().logConfig.value());

    if (daemon) {
        log_debug("Start discovery agent as daemon");
        fty::Daemon::daemonize();
    }

    if (auto res = dis.init()) {
        dis.run();
        dis.shutdown();
    } else {
        log_error(res.error().c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
