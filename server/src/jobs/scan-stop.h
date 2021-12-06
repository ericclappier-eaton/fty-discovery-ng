#pragma once
#include "discovery-task.h"

namespace fty::disco::job {

class ScanStop : public Task<ScanStop, void, commands::scan::stop::Out>
{
public:
    using Task::Task;
    void run(commands::scan::stop::Out& out);
};

} // namespace fty::disco::job
