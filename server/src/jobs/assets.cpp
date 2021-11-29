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

#include "assets.h"
#include "impl/mibs.h"
#include "impl/nut/mapper.h"
#include "impl/nut/process.h"
#include "impl/ping.h"
#include "impl/uuid.h"
#include <fty/string-utils.h>

namespace fty::job {

Expected<void> Assets::getAssets(const commands::assets::In& in, commands::assets::Out& out)
{
    if (!available(in.address)) {
        return unexpected("Host is not available: {}", in.address.value());
    }

    m_params = in;

    // Workaround to check if snmp is available. Read mibs from asset
    if (m_params.protocol == "nut_snmp") {
        if (!m_params.port.hasValue()) {
            m_params.port = 161;
        }

        impl::MibsReader reader(m_params.address, uint16_t(m_params.port.value()));

        if (m_params.settings.credentialId.hasValue()) {
            if (auto res = reader.setCredentialId(m_params.settings.credentialId); !res) {
                return unexpected(res.error());
            }
        } else if (m_params.settings.community.hasValue()) {
            if (auto res = reader.setCommunity(m_params.settings.community); !res) {
                return unexpected(res.error());
            }
        } else {
            return unexpected("Credential or community must be set");
        }

        logDebug("Reading mibs...");
        if (auto mibs = reader.read(); !mibs) {
            logDebug("mibs not found ({})", mibs.error());
            return unexpected(mibs.error());
        } else {
            if (!m_params.settings.mib.hasValue()) {
                m_params.settings.mib = *mibs->begin();
            }
        }
    }

    // Runs nut process
    impl::nut::Process proc(m_params.protocol);

    if (auto res = proc.init(m_params.address, uint16_t(m_params.port.value()))) {
        if (m_params.settings.credentialId.hasValue()) {
            if (auto set = proc.setCredentialId(m_params.settings.credentialId); !set) {
                return unexpected(set.error());
            }
        } else if (m_params.settings.community.hasValue()) {
            if (auto set = proc.setCommunity(m_params.settings.community); !set) {
                return unexpected(set.error());
            }
        }

        if (m_params.settings.username.hasValue() && m_params.settings.password.hasValue()) {
            proc.setCredential(m_params.settings.username, m_params.settings.password);
        }

        if (m_params.settings.timeout.hasValue()) {
            proc.setTimeout(m_params.settings.timeout);
        }

        if (m_params.settings.mib.hasValue()) {
            proc.setMib(m_params.settings.mib);
        }

        if (auto cnt = proc.run()) {
            parse(*cnt, out);
        } else {
            return unexpected(cnt.error());
        }
    } else {
        return unexpected(res.error());
    }
    return {};
}


void Assets::run(const commands::assets::In& in, commands::assets::Out& out)
{
    auto res = getAssets(in, out);
    if (!res) {
        throw Error(res.error());
    }
}

void Assets::addSensors(const DeviceInfo& deviceInfo, int& indexSensor, pack::ObjectList<fty::commands::assets::Asset>& sensors, const std::map<std::string, std::string>& dump)
{
    // Ambient sensor(s)
    bool isSensorsIndexed = false;
    int startSensor = 0, endSensor = 0;
    {
        // First, check for new style and daisychained sensors
        std::string ambientCount = "ambient.count";
        if (deviceInfo.indexDevice != 0) {
            // for daisy chained devices, the ambient count is stored in device.1.ambient.count
            ambientCount = "device.1.ambient.count";
        }
        auto item = dump.find(ambientCount);
        if (item != dump.end()) {
            startSensor = indexSensor;
            endSensor   = fty::convert<int>(item->second);
            isSensorsIndexed = true;
        } else {
            // Otherwise, fallback to checking for legacy sensors
            // First, use "ambient.present" if available
            item = dump.find("ambient.present");
            if (item != dump.end() && item->second != "no") {
                startSensor = endSensor = 1;
            } else {
                // Otherwise, fallback to checking "ambient.temperature" presence
                item = dump.find("ambient.temperature");
                if (item != dump.end()) {
                    startSensor = endSensor = 1;
                }
            }
        }
    }

    logDebug("Discovered {} sensor(s) - ({} to  {})", endSensor, startSensor, endSensor);

    for (int s = startSensor; s <= endSensor; s++) {
        std::string prefix;
        if (deviceInfo.indexDevice > 0) {
            if (isSensorsIndexed) {
                prefix = "device.1.";
            }
            else {
                prefix = "device." + std::to_string(deviceInfo.indexDevice) + ".";
            }
        }
        prefix += "ambient.";
        if (isSensorsIndexed) {
            prefix += std::to_string(s) + ".";
        }

        logTrace("sensor s={} prefix={}", s, prefix);

        // check parent serial number
        std::string parentSerial;
        auto it_parent_serial = dump.find(prefix + "parent.serial");
        if (it_parent_serial != dump.end()) {
            parentSerial = it_parent_serial->second;
        }
        // if daisy chained with indexed sensors (EMP02)
        if ((deviceInfo.indexDevice != 0) && isSensorsIndexed) {
            // Check if it is the good parent which host the sensor
            if (parentSerial.empty() || parentSerial != deviceInfo.deviceSerial) {
                logDebug("No the good parent for sensor {}: {} (parent sensor) <> {} (device) ", s, parentSerial, deviceInfo.deviceSerial);
                continue;
            }
        }

        // check if sensor is physically present (may be reported in NUT scan, but not present)
        auto it_present = dump.find(prefix + "present");
        if (it_present == dump.end()) {
            logError("No present field for sensor number {}", s);
            continue;
        }
        if (it_present->second != "yes") {
            logWarn("Sensor {} is not present", s);
            continue;
        }

        // get sensor serial number (mandatory)
        std::string sensorSerialNumber;
        auto it_sensor_serial = dump.find(prefix + "serial");
        if (it_sensor_serial == dump.end()) {
            logError("No serial number for sensor number {}", s);
            continue;
        }
        sensorSerialNumber = it_sensor_serial->second;

        // check if sensor was already discovered
        if (auto f = m_discoveredSerialSet.find(sensorSerialNumber); f != m_discoveredSerialSet.end()) {
            logWarn("Sensor {} already discovered. Skipping", sensorSerialNumber);
            continue;
        } else {
            // add sensor serial to set of discovered devices
            m_discoveredSerialSet.insert(sensorSerialNumber);
        }

        // look for sensor model (mandatory)
        std::string sensorModel;
        auto it_model = dump.find(prefix + "model");
        // model field could be not present, if there is parent_serial field -> EMP002
        if (it_model != dump.end()) {
            sensorModel = it_model->second;
            // some devices report Eaton EMPDT1H1C2 instead of EMPDT1H1C2
            if (sensorModel.compare("Eaton EMPDT1H1C2") == 0) {
                sensorModel = "EMPDT1H1C2";
            }
        } else if (!parentSerial.empty()) {
            sensorModel = "EMPDT1H1C2";
        } else {
            logError("No model for sensor number {}", s);
            continue;
        }

        // look for manufacturer (mandatory)
        std::string sensorManufacturer;
        auto it_mfr = dump.find(prefix + "mfr");
        if (it_mfr != dump.end()) {
            sensorManufacturer = it_mfr->second;
        } else {
            logError("No manufacturer for sensor number {}", s);
            continue;
        }

        // define parent name (parent_name.1 field)
        std::string parentName = deviceInfo.deviceSerial;
        if (parentName.empty()) {
            std::string ip(m_params.address);
            parentName = deviceInfo.type + " (" + ip + ")";
        }

        // set parent identifier as the first available of:
        // - parentSerial
        // - deviceSerial
        // - ip
        std::string parentIdentifier = [&]() {
            if (!parentSerial.empty()) {
                return parentSerial;
            }
            if (!deviceInfo.deviceSerial.empty()) {
                return deviceInfo.deviceSerial;
            }
            std::string ip(m_params.address);
            return ip;
        }();

        std::string modbusAddress;
        // get modbus address (mandatory for EMPDT1H1C2)
        if (sensorModel.compare("EMPDT1H1C2") == 0) {
            // look for modbus address (mandatory)
            auto it_modbus_addr = dump.find(prefix + "address");
            if (it_modbus_addr != dump.end()) {
                modbusAddress = it_modbus_addr->second;
            } else {
                logError("No modbus address for sensor number {}", s);
                continue;
            }
        }

        // set unique sensor external name
        // default name may be duplicated (i.e., EMPDT1H1C2 @1)
        std::string externalName = "sensor " + sensorModel + " (" + sensorSerialNumber + ")";

        auto& sensor = sensors.append();
        indexSensor ++;

        sensor.name = sensorSerialNumber;  // TBD
        sensor.type = "device";
        sensor.subtype = "sensor";

        // Update ext values
        for (const auto& p : dump) {
            if (p.first.find(prefix) == std::string::npos) {
                continue;
            }

            if (auto key = impl::nut::Mapper::mapKey(p.first, s); !key.empty()) {
                addAssetVal(sensor, key, p.second);
            }
        }

        // Update asset name
        logTrace("Set asset name={} for sensor {}", externalName, s);
        if (!getAssetVal(sensor, "name")) {
            addAssetVal(sensor, "name", externalName);
        }
        else {
            if (auto index = sensor.ext.findIndex([](auto& map) { return map.contains("name"); }); index != -1) {
                sensor.ext[index]["name"] = externalName;
            }
        }
        addAssetVal(sensor, "model", sensorModel);
        addAssetVal(sensor, "endpoint.1.sub_address", modbusAddress);

        logDebug("Added new sensor ({}/{}): SERIAL: {} - TYPE: {} - PARENT: {}", s, endSensor, sensorSerialNumber, sensorModel, parentIdentifier);
    }
}

void Assets::parse(const std::string& cnt, commands::assets::Out& out)
{
    static std::regex rex("([A-Za-z0-9\\.]+)\\s*:\\s+(.*)");

    std::map<std::string, std::string> tmpMap;

    log_debug(cnt.c_str());

    std::stringstream ss(cnt);
    for (std::string line; std::getline(ss, line);) {
        auto [key, value] = fty::split<std::string, std::string>(line, rex);
        if (!key.empty()) {
            tmpMap.emplace(key, value);
        }
    }

    int indexSensor = 1;

    //Get the device type
    auto it = tmpMap.find("device.type");
    std::string deviceType = it != tmpMap.end() ? fty::convert<std::string>(it->second) : "";

    it = tmpMap.find("device.count");

    int dcount = it != tmpMap.end() ? fty::convert<int>(it->second) : 0;
    if (dcount > 1) { //daisy chain is always bigger than one
        // for daisychain
        for (int i = 1; i <= dcount; i++) {
            auto& asset      = out.append();
            asset.subAddress = std::to_string(i);
            asset.asset.type = "device";
            asset.asset.subtype = deviceType;

            std::string prefix = "device." + std::to_string(i) + ".";
            for (const auto& p : tmpMap) {
                // Filter no current daisy device key and daisy ambient key (post treatment with addSensors function)
                static std::regex rex2("device\\.(\\d+)\\.(.*)");
                std::smatch matches;
                if (std::regex_match(p.first, matches, rex2)) {
                    if (matches.str(1) != std::to_string(i) || matches.str(2).find("ambient.") != std::string::npos) {
                        logTrace("Filter property '{}' index={} (daisy-chain override)", p.first, i);
                        continue;
                    }
                }

                // Let daisy-chained device data override host device data (device.<id>.<property> => device.<property>)
                static std::regex overrideRegex(R"xxx((device|ambient)\.([^[:digit:]].*))xxx", std::regex::optimize);
                if (std::regex_match(p.first, matches, overrideRegex)) {
                    if (tmpMap.count(matches.str(1) + "." + std::to_string(i) + "." + matches.str(2))) {
                        logTrace("Ignoring overriden property '{}' (daisy-chain override)", p.first);
                        continue;
                    }
                }

                if (auto key = impl::nut::Mapper::mapKey(p.first, i); !key.empty()) {
                    addAssetVal(asset.asset, key, p.second);
                }
            }
            enrichAsset(asset);
    else {

            // Get serial number of device
            std::string deviceSerial;
            auto it_serial = tmpMap.find(prefix + "serial");
            if (it_serial != tmpMap.end()) {
                deviceSerial = it_serial->second;
            }

            // Set device information
            DeviceInfo deviceInfo = {
                i,
                deviceSerial,
                deviceType
            };

            // Add sensor assets
            addSensors(deviceInfo, indexSensor, asset.sensors, tmpMap);
        }
    } 
    else {
        auto& asset      = out.append();
        asset.subAddress = "";
        asset.asset.type = "device";
        asset.asset.subtype = deviceType;

        for (const auto& p : tmpMap) {
            // Filter ambient key (post treatment with addSensors function)
            if (p.first.find("ambient.") != std::string::npos) {
                continue;
            }
            if (auto key = impl::nut::Mapper::mapKey(p.first); !key.empty()) {
                addAssetVal(asset.asset, key, p.second);
            }
        }
        enrichAsset(asset);

        std::string deviceSerial;
        auto it_serial = tmpMap.find("device.serial");
        if (it_serial != tmpMap.end()) {
            deviceSerial = it_serial->second;
        }

        // Set device information
        DeviceInfo deviceInfo = {
            0,
            deviceSerial,
            deviceType
        };

        // Add sensor assets
        addSensors(deviceInfo, indexSensor, asset.sensors, tmpMap);
    }
}

void Assets::addAssetVal(commands::assets::Asset& asset, const std::string& key, const std::string& val, bool readOnly)
{
    auto& ext = asset.ext.append();
    ext.append(key, val);
    ext.append("read_only", (readOnly ? "true" : "false"));
}

fty::Expected<std::string> Assets::getAssetVal(const commands::assets::Asset& asset, const std::string& key) const
{
    if (auto it = asset.ext.find([key](const auto& map) { return map.contains(key); }); it != std::nullopt) {
        return (*it)[key];
    }
    return fty::unexpected("Not found");
}

void Assets::enrichAsset(commands::assets::Return& asset)
{
    if (asset.asset.subtype.empty()) {
        if (auto type = getAssetVal(asset.asset, "device.type")) {
            asset.asset.subtype = *type;
        }
    }

    if (asset.asset.subtype == "pdu") {
        asset.asset.subtype = "epdu";
    }
    if (asset.asset.subtype == "ats") {
        asset.asset.subtype = "sts";
    }

    addAssetVal(asset.asset, "ip.1", m_params.address, false);

    // uuid attribute
    {
        auto manufacturer = getAssetVal(asset.asset, "manufacturer");
        auto model = getAssetVal(asset.asset, "model");
        auto serial = getAssetVal(asset.asset, "serial_no");
        std::string uuid; //empty
        if (manufacturer && model && serial) {
            uuid = fty::impl::generateUUID(*manufacturer, *model, *serial), false);
        }
        addAssetVal(asset.asset, "uuid", uuid, false);
    }

    // max_power attribute
    {
        std::string max_power; //empty
        auto realpower_nominal = getAssetVal(asset.asset, "realpower.nominal");
        if (realpower_nominal) {
            max_power = *realpower_nominal;
        } else {
            //try to get realpower.default.nominal 
            auto realpower_default_nominal = getAssetVal(asset.asset, "realpower.default.nominal");
            if (realpower_default_nominal) {
                max_power = *realpower_default_nominal;
            }
        }
        if (!max_power.empty()) {
            addAssetVal(asset.asset, "max_power", max_power, false);
        }
    }

    // set external name
    std::string name;
    if (auto hostName = getAssetVal(asset.asset, "hostname")) {
        name = *hostName;
    } else {
        if (!asset.asset.subtype.empty()) {
            std::stringstream ss;
            ss << std::string(asset.asset.subtype) << " (" << std::string(m_params.address) << ")";
            name = ss.str();
        } else {
            name = m_params.address;
        }
    }

    // epdu daisy_chaine attribut
    if (asset.asset.subtype == "epdu") {
        std::string daisyChain = "0";
        if (!asset.subAddress.empty()) {
            daisyChain = asset.subAddress;
        }
        addAssetVal(asset.asset, "daisy_chain", daisyChain);

        // Redefine name
        if (daisyChain != "0") {
            std::stringstream ss;
            if (daisyChain == "1") {
                ss << name << " Host";
            } else {
                int num = std::stoi(daisyChain) - 1;
                ss << name << " Device" << num;
            }
            name = ss.str();
        }
    }
    logTrace("Set asset name={}", name);
    if (!getAssetVal(asset.asset, "name")) {
        addAssetVal(asset.asset, "name", name);
    } else {
        if (auto index = asset.asset.ext.findIndex([](const auto& map) {
                return map.contains("name");
            });
            index != -1) {
            asset.asset.ext[index]["name"] = name;
        }
    }
    // endpoint.1 attributes (monitoring)
    addAssetVal(asset.asset, "endpoint.1.protocol", m_params.protocol, false);
    addAssetVal(asset.asset, "endpoint.1.port", std::to_string(m_params.port), false);
    addAssetVal(asset.asset, "endpoint.1.sub_address", asset.subAddress, false);
    addAssetVal(asset.asset, "endpoint.1.status.operating", "IN_SERVICE", false);
    addAssetVal(asset.asset, "endpoint.1.status.error_msg", "", false);
    if (m_params.protocol == "nut_snmp") {
        if (m_params.settings.credentialId.hasValue()) {
            addAssetVal(asset.asset, "endpoint.1.nut_snmp.secw_credential_id", m_params.settings.credentialId, false);
        }
        if (m_params.settings.community.hasValue()) {
            addAssetVal(asset.asset, "endpoint.1.nut_snmp.community", m_params.settings.community, false);
        }
        if (m_params.settings.mib.hasValue()) {
            addAssetVal(asset.asset, "endpoint.1.nut_snmp.MIB", m_params.settings.mib, false);
        }
    }

    // endpoint.2 attributes (mass management)
    {
        std::string port = "443"; // secure port default
        if (m_params.protocol == "nut_powercom") {
            port = std::to_string(m_params.port);
        }
        addAssetVal(asset.asset, "endpoint.2.port", port, false);
    }
}

} // namespace fty::job
