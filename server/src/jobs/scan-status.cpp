#include "scan-status.h"
#include "discovery.h"

namespace fty::disco::job {

void ScanStatus::run(commands::scan::status::Out& out)
{
    using namespace commands::scan::status;

    out = m_autoDiscovery->getStatus();
    /*out.status = status.state;
    out.progress = std::to_string(status.progress) + "%";
    out.discovered = status.discovered;
    out.ups = status.ups;
    out.epdu = status.epdu;
    out.sts = status.sts;
    out.sensors = status.sensors;*/
}

} // namespace fty::disco::job
