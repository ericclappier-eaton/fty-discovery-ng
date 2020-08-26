#include "chaineddevices.h"
#include "protocols/snmp.h"
#include "message-bus.h"

namespace fty::job {

ChainedDevices::ChainedDevices(const Message& in, MessageBus& bus):
    m_in(in),
    m_bus(&bus)
{
}

void ChainedDevices::operator()()
{
    (void)m_bus;
}

}
