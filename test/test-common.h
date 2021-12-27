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
#include <mlm_library.h>
#include <mlm_server.h>

// TBD
#include <czmq.h>
#include <fstream>
#include <fty_common_mlm.h>

class Test;
inline Test* inst;

class Test
{
public:
    ~Test()
    {
        //zactor_destroy(&m_broker);
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

    /*static void subscribe(const std::string& queue, std::function<void(const messagebus::Message&)>&& fun)
    {
        return inst->m_bus.subscribe<>(fty::disco::Channel, msg);
    }*/

    static fty::Expected<void> init()
    {
        inst = new Test;
        if (auto ret = inst->m_dis.loadConfig(); !ret) {
            return fty::unexpected("Cannot load config");
        }

        fty::disco::impl::Snmp::instance().init(fty::disco::Config::instance().mibDatabase);
        ManageFtyLog::setInstanceFtylog(fty::disco::Config::instance().actorName, fty::disco::Config::instance().logConfig);

// TBD
#if 0
        // create the broker
        static const char* endpoint_disco = "inproc://fty-discovery-ng-test";
        zactor_t *broker = inst->m_broker;//.get();
        zstr_sendx(inst->m_broker, "BIND", endpoint_disco, NULL);
        zstr_send(inst->m_broker, "VERBOSE");
#endif

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

    fty::disco::Discovery& getDisco() {
        return m_dis;
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
          //m_broker(zactor_new(mlm_server, const_cast<char*>("Malamute")))
    {
    }

private:
    fty::disco::Discovery     m_dis;
    std::thread               m_th;
    fty::disco::MessageBus    m_bus;
    // TBD
    //std::unique_ptr<zactor_t> m_broker;
    //zactor_t*                 m_broker = nullptr;
};
