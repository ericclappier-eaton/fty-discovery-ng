#pragma once
#include "discovery-task.h"

namespace fty::disco::job {

class ScanStart : public Task<ScanStart, commands::scan::start::In, commands::scan::start::Out>
{
public:
    using Task::Task;
    void run(const commands::scan::start::In& in, commands::scan::start::Out& out);
};

} // namespace fty::disco::job
