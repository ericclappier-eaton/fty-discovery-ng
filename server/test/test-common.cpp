#include "test-common.h"

Test::Test() : m_dis("conf/discovery.conf")
{
}

Test::~Test()
{
}

fty::disco::Message Test::createMessage(const char* subject)
{
    fty::disco::Message msg;
    msg.meta.to      = fty::disco::Config::instance().actorName;
    msg.meta.subject = subject;
    msg.meta.from    = "unit-test";
    return msg;
}

fty::Expected<fty::disco::Message> Test::send(const fty::disco::Message& msg)
{
    return Test::instance().m_bus.send(fty::disco::Channel, msg);
}

fty::Expected<void> Test::init()
{
    if (auto ret = Test::instance().m_dis.loadConfig(); !ret) {
        return fty::unexpected("Cannot load config");
    }

    fty::disco::impl::Snmp::instance().init(fty::disco::Config::instance().mibDatabase.value());
    ManageFtyLog::setInstanceFtylog(fty::disco::Config::instance().actorName.value(), "");

    // Create the broker
    static const char* endpoint_disco = "inproc://fty-discovery-ng-test";
    Test::instance().m_broker = zactor_new(mlm_server, const_cast<char*>("Malamute"));
    zstr_sendx(Test::instance().m_broker, "BIND", endpoint_disco, NULL);
    //zstr_send(Test::instance().m_broker, "VERBOSE");

    if (auto res = Test::instance().m_dis.init(); !res) {
        return fty::unexpected(res.error());
    }

    if (auto res = Test::instance().m_bus.init("unit-test", endpoint_disco); !res) {
        return fty::unexpected(res.error());
    }

    Test::instance().m_th = std::thread([&]() {
        Test::instance().m_dis.run();
    });
    return {};
}

fty::disco::Discovery& Test::getDisco() {
    return Test::instance().m_dis;
}

fty::disco::MessageBus& Test::getBus() {
    return Test::instance().m_bus;
}

void Test::shutdown()
{
    Test::instance().m_dis.shutdown();
    Test::instance().m_th.join();
    Test::instance().m_bus.shutdown();
    zactor_destroy(&(Test::instance().m_broker));
}

Test& Test::instance()
{
    static Test test;
    return test;
}
