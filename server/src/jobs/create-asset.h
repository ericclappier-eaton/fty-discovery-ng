/*  =========================================================================
    Copyright (C) 2021 Eaton.

    This software is confidential and licensed under Eaton Proprietary License
    (EPL or EULA).
    This software is not authorized to be used, duplicated or disclosed to
    anyone without the prior written permission of Eaton.
    Limitations, restrictions and exclusions of the Eaton applicable standard
    terms and conditions, such as its EPL and EULA, apply.
    =========================================================================
*/

#pragma once

#include "message-bus.h"
#include "message.h"

namespace fty::disco::job::asset::create {

static constexpr const char* Subject = "CREATE";

class PowerLink : public pack::Node
{
public:
    pack::String source     = FIELD("source");
    pack::UInt32 link_type  = FIELD("link_type");
    pack::String src_out    = FIELD("src_out", "");

    using pack::Node::Node;
    META(PowerLink, source, link_type, src_out);
};

class MapExtObj: public pack::Node
{
public:
    pack::String value     = FIELD("value");
    pack::Bool   readOnly  = FIELD("readOnly");
    pack::Bool   update    = FIELD("update", true);

    using pack::Node::Node;
    META(MapExtObj, value, readOnly, update);
};

using Ext = pack::Map<MapExtObj>;

using PowerLinks = pack::ObjectList<PowerLink>;

class Common : public pack::Node
{
public:
    pack::String name                        = FIELD("name");
    pack::UInt32 status                      = FIELD("status");
    pack::String type                        = FIELD("type");
    pack::String sub_type                    = FIELD("sub_type");
    pack::UInt32 priority                    = FIELD("priority");
    pack::String parent                      = FIELD("parent");
    Ext          ext                         = FIELD("ext");
    PowerLinks   linked                      = FIELD("linked");

    using pack::Node::Node;
    META(Common, name, status, type, sub_type, priority, parent, ext, linked);
};

using Request  = Common;
using Response = Common;

fty::Expected<Response> run(fty::disco::MessageBus& bus, const std::string& from, const Request& req);

} // namespace fty::disco::job::asset::create
