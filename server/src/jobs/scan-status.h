#pragma once
#include "discovery-task.h"

namespace fty::disco::job {

class ScanStatus : public Task<ScanStatus, void, commands::scan::status::Out>
{
public:
    using Task::Task;
    void run(commands::scan::status::Out& out);
};

} // namespace fty::disco::job
