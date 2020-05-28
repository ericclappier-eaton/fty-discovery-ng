#pragma once
#include <string>
#include <fty/event.h>

namespace fty {

class Daemon
{
public:
    static void    daemonize();
    static Daemon& instance();
    static void    handleSignal(int sig);
    static void    stop();

public:
    Event<> stopEvent;
    Event<> loadConfigEvent;

private:
    Daemon();
};

}
