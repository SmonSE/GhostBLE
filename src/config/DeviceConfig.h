#pragma once

#include <Arduino.h>
#include <Preferences.h>

class DeviceConfig {
public:
    void begin();
    String getName() const;
    String getFace() const;
    String getWifiSSID() const;
    String getWifiPassword() const;
    bool set(const String& key, const String& value);
    String handleMessage(const String& msg);

private:
    Preferences prefs;
    String name;
    String face;
    String wifiSSID;
    String wifiPassword;
};

extern DeviceConfig deviceConfig;
