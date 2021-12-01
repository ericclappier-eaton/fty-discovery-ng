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


class Test;
inline Test* inst;

class Test
{
public:
    ~Test()
    {
    }

    static fty::disco::Message createMessage(const char* subject)
    {
        fty::disco::Message msg;
        msg.meta.to      = fty::disco::Config::instance().actorName;
        msg.meta.subject = subject;
        msg.meta.from    = "unit-test";
        return msg;
    }

    static fty::Expected<fty::disco::Message> send(const fty::disco::Message& msg)
    {
        return inst->m_bus.send(fty::disco::Channel, msg);
    }

    static fty::Expected<void> init()
    {
        inst = new Test;
        if (auto ret = inst->m_dis.loadConfig(); !ret) {
            return fty::unexpected("Cannot load config");
        }

        fty::disco::impl::Snmp::instance().init(fty::disco::Config::instance().mibDatabase);
        ManageFtyLog::setInstanceFtylog(fty::disco::Config::instance().actorName, fty::disco::Config::instance().logConfig);

        if (auto res = inst->m_dis.init(); !res) {
            return fty::unexpected(res.error());
        }

        if (auto res = inst->m_bus.init("unit-test"); !res) {
            return fty::unexpected(res.error());
        }

        inst->m_th = std::thread([&]() {
            inst->m_dis.run();
        });
        return {};
    }

    static void shutdown()
    {
        inst->m_dis.shutdown();
        inst->m_th.join();
        delete inst;
    }

private:
    Test()
        : m_dis("conf/discovery.conf")
    {
    }

private:
    fty::disco::Discovery         m_dis;
    std::thread            m_th;
    fty::disco::MessageBus m_bus;
};
