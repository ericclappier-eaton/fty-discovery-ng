#pragma once
#include "config.h"
#include "message-bus.h"
#include <fty/event.h>
#include <string>

namespace fty {

class Discovery
{
public:
    Discovery(const std::string& config);

    bool loadConfig();
    int  run();
    void init();
    void shutdown();

    const Config& config() const;
    Event<> stop;

private:
    void discover(const Message& msg);
    void configure(const Message& msg);
    void details(const Message& msg);
    void doStop();

private:
    std::string m_configPath;
    Config      m_config;
    MessageBus  m_bus;

    Slot<> m_stopSlot       = {&Discovery::doStop, this};
    Slot<> m_loadConfigSlot = {&Discovery::loadConfig, this};

    Slot<const Message&> m_discoverSlot  = {&Discovery::discover, this};
    Slot<const Message&> m_configureSlot = {&Discovery::configure, this};
    Slot<const Message&> m_detailsSlot   = {&Discovery::details, this};
};

} // namespace fty
