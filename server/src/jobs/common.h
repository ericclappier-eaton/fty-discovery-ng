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
#include "message-bus.h"
#include <fty/expected.h>
#include <fty_log.h>
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
    META(BasicInfo, name, mibs, type);
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
    META(BasicResponse, error, status);

public:
    void setError(const std::string& errMsg)
    {
        log_error("Error: %s", errMsg.c_str());
        error  = errMsg;
        status = Message::Status::Error;
    }

    operator Message()
    {
        Message msg;
        msg.meta.status = status;
        if (status == Message::Status::Ok) {
            msg.userData.setString(*pack::json::serialize(static_cast<T*>(this)->data()));
        } else {
            msg.userData.setString(error);
        }
        return msg;
    }
};

// =====================================================================================================================

bool                            filterMib(const std::string& mib);
const std::vector<std::string>& knownMibs();
Expected<BasicInfo>             readSnmp(const std::string& ipAddress, uint16_t port, const std::string& community = {},
                const std::string& secId = {}, uint32_t timeout = 1);
bool                            isSnmp(const std::string& mib);
std::string                     mapMibToLegacy(const std::string& mib);

// =====================================================================================================================

inline std::ostream& operator<<(std::ostream& ss, BasicInfo::Type type)
{
    switch (type) {
        case BasicInfo::Type::Snmp:
            ss << "Snmp";
            break;
        case BasicInfo::Type::Xml:
            ss << "Xml";
            break;
    }
    return ss;
}

inline std::istream& operator>>(std::istream& ss, BasicInfo::Type& type)
{
    std::string str;
    ss >> str;
    if (str == "Snmp") {
        type = BasicInfo::Type::Snmp;
    } else if (str == "Xml") {
        type = BasicInfo::Type::Xml;
    }
    return ss;
}

} // namespace fty::job
