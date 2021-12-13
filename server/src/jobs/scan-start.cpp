#include "scan-start.h"
#include "discovery.h"

namespace fty::disco::job {

void ScanStart::run(const commands::scan::start::In& in, commands::scan::start::Out& out)
{
    if (auto res = m_autoDiscovery->start(in); !res) {
        throw Error("Error when started automatic discovery: {}", res.error());
    }

    // no output
}

} // namespace fty::disco::job
