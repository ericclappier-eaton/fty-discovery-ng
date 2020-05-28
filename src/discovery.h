#pragma once
#include <fty/event.h>
#include <string>
#include "config.h"

namespace fty {

class Discovery
{
public:
    static constexpr const char* ActorName = "discovery-ng";
public:
    Discovery(const std::string& config);

    void stop();
    bool loadConfig();
    int run();

    const Config& config() const;

private:
    std::string m_configPath;
    Config      m_config;
    Slot<>      m_stopSlot       = {&Discovery::stop, this};
    Slot<>      m_loadConfigSlot = {&Discovery::loadConfig, this};
};

} // namespace fty
