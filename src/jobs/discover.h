#pragma once
#include "src/message.h"
#include <fty/thread-pool.h>
#include <fty/expected.h>

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
    Expected<bool> portIsOpen(const std::string& address, uint16_t port);
private:
    Message     m_in;
    MessageBus* m_bus;
};

} // namespace fty::job
