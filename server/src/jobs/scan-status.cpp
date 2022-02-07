#include "scan-status.h"
#include "discovery.h"

namespace fty::disco::job {

void ScanStatus::run(commands::scan::status::Out& out)
{
    using namespace commands::scan::status;

    out = m_autoDiscovery->getStatus();
}

} // namespace fty::disco::job
