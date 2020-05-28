#pragma once
#include <pack/pack.h>

namespace fty {

class Config: public pack::Node
{
public:
    using pack::Node::Node;

    pack::String logConfig = FIELD("log-config", "conf/logger.conf");

    META_FIELDS(Config, logConfig)
};

}
