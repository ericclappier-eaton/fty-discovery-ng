#include "discover.h"
#include "src/message-bus.h"


namespace fty::job {

Discover::Discover(const Message& in, MessageBus& bus)
    : m_in(in)
    , m_bus(&bus)
{
}

void Discover::operator()()
{
    Message reply;
    reply.meta.to      = m_in.meta.from;
    reply.meta.from    = m_in.meta.to;
    reply.meta.subject = "reply";
    reply.meta.status  = Message::Status::ok;
    reply.userData.append("reply from thread");
    reply.userData.append(m_in.userData[1]);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1000ms);
    m_bus->reply("discover", m_in, reply);
}

} // namespace fty::job
