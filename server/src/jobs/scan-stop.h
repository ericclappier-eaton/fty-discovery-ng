#pragma once
#include "discovery-task.h"

namespace fty::disco::job {

class ScanStop : public AutoTask<ScanStop, void, commands::scan::stop::Out>
{
public:
    using AutoTask::AutoTask;

    // Runs discover job.
    void run(commands::scan::stop::Out&);
};

} // namespace fty::disco::job
