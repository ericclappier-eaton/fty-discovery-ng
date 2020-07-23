/*  =========================================================================
    common.h - Jobs common stuff

    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
 */

#pragma once
#include "src/message-bus.h"
#include <fty/expected.h>
#include <pack/pack.h>

namespace fty::job {

// =====================================================================================================================

/// Basic information for discovery
class BasicInfo : public pack::Node
{
public:
    enum class Type
    {
        Snmp,
        Xml
    };

    pack::String     name = FIELD("name");
    pack::StringList mibs = FIELD("mibs");
    pack::Enum<Type> type = FIELD("type");

public:
    using pack::Node::Node;
    META_FIELDS(BasicInfo, name, mibs, type)
};

// =====================================================================================================================

/// Basic responce wrapper
template <typename T>
class BasicResponse : public pack::Node
{
public:
    pack::String                error  = FIELD("error");
    pack::Enum<Message::Status> status = FIELD("status");

public:
    using pack::Node::Node;
    META_FIELDS(Response, error, status)

public:
    void setError(const std::string& errMsg)
    {
        error = errMsg;
        status = Message::Status::ko;
    }

    operator Message()
    {
        Message msg;
        msg.meta.status = status;
        if (status == Message::Status::ok) {
            msg.userData.append(*pack::json::serialize(static_cast<T*>(this)->data()));
        } else {
            msg.userData.append(*pack::json::serialize(error));
        }
        return msg;
    }
};

// =====================================================================================================================

bool                            filterMib(const std::string& mib);
const std::vector<std::string>& knownMibs();
Expected<BasicInfo>             readSnmp(const std::string& ipAddress);

// =====================================================================================================================

} // namespace fty::job
