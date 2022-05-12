#pragma once

#include "commands.h"
#include "message-bus.h"
#include "message.h"
#include "src/config.h"
#include "src/discovery.h"
#include "src/jobs/impl/snmp.h"
#include <catch2/catch.hpp>
#include <fty_log.h>
#include <thread>
#include <malamute.h>

class Test
{
public:
    Test();
    ~Test();

    static fty::disco::Message createMessage(const char* subject);

    static fty::Expected<fty::disco::Message> send(const fty::disco::Message& msg);

    static fty::Expected<void> init();

    static fty::disco::Discovery& getDisco();

    static fty::disco::MessageBus& getBus();

    static void shutdown();

    static Test& instance();

private:
    fty::disco::Discovery     m_dis;
    zactor_t*                 m_broker = nullptr;
    std::thread               m_th;
    fty::disco::MessageBus    m_bus;
};
