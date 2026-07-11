#pragma once
#include <Preferences.h>

class MenuSettings {
public:
    void begin();
    void save();

private:
    Preferences prefs;
};

extern MenuSettings menuSettings;