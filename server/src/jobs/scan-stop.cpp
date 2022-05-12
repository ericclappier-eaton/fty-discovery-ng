#include "scan-stop.h"
#include "discovery.h"


namespace fty::disco::job {

void ScanStop::run(commands::scan::stop::Out&)
{
    if (auto res = m_autoDiscovery->stop(); !res) {
        throw Error("Error when stopped automatic discovery: {}", res.error());
    }
}

} // namespace fty::disco::job
