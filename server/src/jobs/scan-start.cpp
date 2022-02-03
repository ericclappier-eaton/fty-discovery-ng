#include "scan-start.h"
#include "discovery.h"

namespace fty::disco::job {

void ScanStart::run(commands::scan::start::Out&)
{
    if (auto res = m_autoDiscovery->start(); !res) {
        throw Error("Error when started automatic discovery: {}", res.error());
    }

    // no output
}

} // namespace fty::disco::job
