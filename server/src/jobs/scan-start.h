#pragma once
#include "discovery-task.h"

namespace fty::disco::job {

class ScanStart : public AutoTask<ScanStart, commands::scan::start::In, commands::scan::start::Out>
{
public:
    using AutoTask::AutoTask;

    // Runs discover job.
    void run(const commands::scan::start::In& in, commands::scan::start::Out&);
};

} // namespace fty::disco::job
