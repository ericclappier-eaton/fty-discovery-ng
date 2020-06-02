#pragma once
#include "src/message.h"
#include <fty/thread-pool.h>

namespace fty {
class MessageBus;
} // namespace fty

namespace fty::job {

class Discover : public Task<Discover>
{
public:
    Discover(const Message& in, MessageBus& bus);

    void operator()() override;

private:
    Message     m_in;
    MessageBus* m_bus;
};

} // namespace fty::job
