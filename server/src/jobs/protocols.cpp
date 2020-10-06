/*  ====================================================================================================================
    Copyright (C) 2020 Eaton
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
    ====================================================================================================================
*/

#include "protocols.h"
#include "impl/mibs.h"
#include "impl/ping.h"
#include "impl/xml-pdc.h"
#include <fty/split.h>
#include <set>

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

// =====================================================================================================================


void Protocols::run(const commands::protocols::In& in, commands::protocols::Out& out)
{
    if (in.address == "__fake__") {
        out.setValue({"NUT_SNMP", "NUT_XML_PDC"});
        return;
    }

    if (!available(in.address)) {
        throw Error("Host is not available: {}", in.address.value());
    }

    std::vector<BasicInfo> protocols;

    if (auto res = tryXmlPdc(in)) {
        protocols.emplace_back(*res);
        log_info("Found XML  device: '%s' mibs[]", res->name.value().c_str());
    } else {
        log_info("Skipped xml_pdc, reason: %s", res.error().c_str());
    }

    if (auto res = trySnmp(in)) {
        protocols.emplace_back(*res);
        log_info("Found SNMP device: '%s' mibs[%s]", res->name.value().c_str(), implode(res->mibs, ", ").c_str());
    } else {
        log_info("Skipped snmp, reason: %s", res.error().c_str());
    }

    sortProtocols(protocols);

    for (const auto& prot : protocols) {
        switch (prot.type) {
            case BasicInfo::Type::Snmp:
                out.append("NUT_SNMP");
                break;
            case BasicInfo::Type::Xml:
                out.append("NUT_XML_PDC");
                break;
        }
    }
}

Expected<BasicInfo> Protocols::tryXmlPdc(const commands::protocols::In& in) const
{
    protocol::XmlPdc xml(in.address);
    if (auto prod = xml.get<protocol::ProductInfo>("product.xml")) {
        if (prod->protocol == "XML.V4") {
            return unexpected("unsupported XML.V4");
        }

        if (auto props = xml.get<protocol::Properties>(prod->summary.summary.url)) {
            BasicInfo info;

            if (auto res = props->value("UPS.PowerSummary.iProduct")) {
                info.name = *res;
            }

            if (auto res = props->value("UPS.PowerSummary.iModel")) {
                info.name = info.name.value() + (info.name.empty() ? "" : " ") + *res;
            }

            info.type = BasicInfo::Type::Xml;
            return std::move(info);
        } else {
            return unexpected(props.error());
        }
    } else {
        return unexpected(prod.error());
    }
}

Expected<BasicInfo> Protocols::trySnmp(const commands::protocols::In& in) const
{
    BasicInfo info;

    protocol::MibsReader reader(in.address, 161);
    reader.setCommunity("public");

    if (auto mibs = reader.read(); !mibs) {
        return unexpected(mibs.error());
    } else {
        info.mibs.setValue(std::vector<std::string>(mibs->begin(), mibs->end()));
    }

    if (auto name = reader.readName(); !name) {
        return unexpected(name.error());
    } else {
        info.name = *name;
    }

    return std::move(info);
}

void Protocols::sortProtocols(std::vector<BasicInfo>& protocols)
{
    static std::set<std::string> epdus = {
        "EATON-EPDU-MIB", "EATON-OIDS", "EATON-GENESIS-II-MIB", "EATON-EPDU-PU-SW-MIB", "ACS-MIB"};

    static std::set<std::string> atss = {"EATON-OIDS", "PowerNet-MIB"};

    auto isMib = [](const BasicInfo& info, const std::set<std::string>& mibs) {
        for (const auto& mib : info.mibs) {
            if (mibs.count(mib)) {
                return true;
            }
        }
        return false;
    };

    std::sort(protocols.begin(), protocols.end(), [&](const BasicInfo& l, const BasicInfo& r) {
        if (l.type == BasicInfo::Type::Snmp && (isMib(l, epdus) || isMib(l, atss))) {
            return false;
        }
        if (r.type == BasicInfo::Type::Snmp && (isMib(r, epdus) || isMib(r, atss))) {
            return true;
        }
        return true;
    });
}


// =====================================================================================================================

} // namespace fty::job
