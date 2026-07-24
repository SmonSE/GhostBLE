#pragma once

#include <Arduino.h>
#include <Preferences.h>

class DeviceConfig {
public:
    void begin();
    bool getFirstBootDone() const;
    void setFirstBootDone(bool v);
    String getName() const;
    String getFace() const;
    String getWifiSSID() const;
    String getWifiPassword() const;
    bool set(const String& key, const String& value);
    String handleMessage(const String& msg);
    bool getStealthMode() const;
    void setStealthMode(bool v);
    String getEffectiveBleName() const;

private:
    Preferences prefs;
    String name;
    String face;
    String wifiSSID;
    String wifiPassword;
    bool   stealthMode = false;
    bool   firstBootDone = false;
};
