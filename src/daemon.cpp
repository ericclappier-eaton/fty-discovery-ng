/*  ========================================================================
    Copyright (C) 2020 Eaton
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    ========================================================================
*/
#include "daemon.h"
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fty {

Daemon::Daemon()
{
    signal(SIGINT, &Daemon::handleSignal);
    signal(SIGHUP, &Daemon::handleSignal);
}

void Daemon::daemonize()
{
    pid_t pid = 0;

    auto doFork = [&]() {
        // Fork off the parent process
        pid = fork();

        // An error occurred
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }

        // Success: Let the parent terminate
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
    };

    // First fork
    doFork();

    // The child process becomes session leader
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Fork off for the second time
    doFork();

    // Set new file permissions
    umask(0);

    // Change the working directory to the root directory or another appropriated directory
    //chdir("/");

    // Close all open file descriptors
    for (long fd = sysconf(_SC_OPEN_MAX); fd > 0; --fd) {
        close(int(fd));
    }


    // Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2)
    stdin  = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");
}

Daemon& Daemon::instance()
{
    static Daemon inst;
    return inst;
}

void Daemon::handleSignal(int sig)
{
    switch (sig) {
        case SIGINT:
            stop();
            break;
        case SIGHUP:
            instance().loadConfigEvent();
            break;
    }
}

void Daemon::stop()
{
    instance().stopEvent();
    signal(SIGINT, SIG_DFL);
}

} // namespace fty
