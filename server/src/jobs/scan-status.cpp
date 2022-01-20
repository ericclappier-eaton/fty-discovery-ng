#include "scan-status.h"
#include "discovery.h"

namespace fty::disco::job {

void ScanStatus::run(commands::scan::status::Out& out)
{
    using namespace commands::scan::status;

    auto status = m_autoDiscovery->getStatus();
    switch (status.state) {
        case AutoDiscovery::State::CancelledByUser:
            out.status = Out::Status::CancelledByUser;
            break;
        case AutoDiscovery::State::Terminated:
            out.status = Out::Status::Terminated;
            break;
        case AutoDiscovery::State::InProgress:
            out.status = Out::Status::InProgress;
            break;
        case AutoDiscovery::State::Unknown:
            out.status = Out::Status::Unknown;
            break;
    }
    out.progress = std::to_string(status.progress) + "%";
    out.discovered = status.discovered;
    out.ups = status.ups;
    out.epdu = status.epdu;
    out.sts = status.sts;
    out.sensors = status.sensors;
}

} // namespace fty::disco::job
