#include "DeviceConfig.h"
#include "logger/logger.h"

DeviceConfig deviceConfig;

void DeviceConfig::begin() {
    prefs.begin("ghostble", true);  // read-only
    name = prefs.getString("name", "NibBLEs");
    face = prefs.getString("face", "(◕‿◕)");
    wifiSSID = prefs.getString("wifiSSID", "GhostBLE");
    wifiPassword = prefs.getString("wifiPass", "ghostble123!");
    prefs.end();
    LOG(LOG_SYSTEM, "Device name: " + name + "  face: " + face);
}

String DeviceConfig::getName() const { return name; }
String DeviceConfig::getFace() const { return face; }
String DeviceConfig::getWifiSSID() const { return wifiSSID; }
String DeviceConfig::getWifiPassword() const { return wifiPassword; }

bool DeviceConfig::set(const String& key, const String& value) {
    if (key == "NAME" && value.length() <= 20) {
        name = value;
    } else if (key == "FACE" && value.length() <= 20) {
        face = value;
    } else if (key == "SSID" && value.length() <= 32) {
        wifiSSID = value;
    } else if (key == "PASS" && value.length() >= 8 && value.length() <= 63) {
        wifiPassword = value;
    } else {
        return false;
    }

    prefs.begin("ghostble", false);
    size_t written = 0;
    if (key == "NAME") written = prefs.putString("name", name);
    else if (key == "FACE") written = prefs.putString("face", face);
    else if (key == "SSID") written = prefs.putString("wifiSSID", wifiSSID);
    else if (key == "PASS") written = prefs.putString("wifiPass", wifiPassword);
    prefs.end();

    if (written == 0) {
        LOG(LOG_CONTROL, "NVS write FAILED for " + key);
        return false;
    }

    LOG(LOG_CONTROL, key + " set: " + (key == "PASS" ? "***" : value));
    return true;
}

String DeviceConfig::handleMessage(const String& msg) {
    if (msg == "GET_CONFIG") {
        return "CONFIG:" + name + "\t" + face + "\t" + wifiSSID;
    }

    if (!msg.startsWith("SET_")) return "";

    int colonIdx = msg.indexOf(':');
    if (colonIdx < 0) return "";

    String key = msg.substring(4, colonIdx);
    String value = msg.substring(colonIdx + 1);
    value.trim();

    if (value.length() == 0 || !set(key, value)) return "";

    return key + "_SET:" + value;
}
