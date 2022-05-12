#pragma once
#include "discovery-task.h"

namespace fty::disco::job {

class ScanStatus : public AutoTask<ScanStatus, void, commands::scan::status::Out>
{
public:
    using AutoTask::AutoTask;
    void run(commands::scan::status::Out& out);
};

} // namespace fty::disco::job
