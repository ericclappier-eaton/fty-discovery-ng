#pragma once
#include "message.h"
#include <fty/expected.h>
#include <fty/thread-pool.h>

namespace fty {
class MessageBus;
} // namespace fty

namespace fty::job {

class ChainedDevices: public Task<ChainedDevices>
{
public:
    ChainedDevices(const Message& in, MessageBus& bus);

    void operator()() override;

private:
    Message     m_in;
    MessageBus* m_bus;
};

}

