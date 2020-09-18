#pragma once
#include <pack/pack.h>
#include "commands.h"

namespace fty::protocol {

fty::Expected<commands::assets::Out> properties(const commands::assets::In& cmd);

}
