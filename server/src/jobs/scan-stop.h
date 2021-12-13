#pragma once
#include "discovery-task.h"

namespace fty::disco::job {

class ScanStop : public AutoTask<ScanStop, void, commands::scan::stop::Out>
{
public:
    using AutoTask::AutoTask;
    void run(commands::scan::stop::Out& out);
};

} // namespace fty::disco::job
