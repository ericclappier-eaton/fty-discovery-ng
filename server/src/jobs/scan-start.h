#pragma once
#include "discovery-task.h"

namespace fty::disco::job {

class ScanStart : public AutoTask<ScanStart, void, commands::scan::start::Out>
{
public:
    using AutoTask::AutoTask;

    // Runs discover job.
    void run(commands::scan::start::Out&);
};

} // namespace fty::disco::job
