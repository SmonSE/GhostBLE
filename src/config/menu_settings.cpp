#include "menu_settings.h"
#include "ui/menu/menu_controller.h"
#include "infrastructure/logging/logger.h"

MenuSettings menuSettings;

static const char* NS = "ghostble_menu";

void MenuSettings::begin() {
    prefs.begin(NS, true);

    MenuController::setAudioEnabled   (prefs.getBool("audioEn",   true));
    MenuController::setAudioSuspicious(prefs.getBool("audioSusp", true));
    MenuController::setAudioFlock     (prefs.getBool("audioFlock",true));
    MenuController::setAudioDrone     (prefs.getBool("audioDrone",true));
    MenuController::setAudioFlipper   (prefs.getBool("audioFlip", true));
    MenuController::setAudioPwnBeacon (prefs.getBool("audioPwn",  true));

    uint8_t alarmVol = prefs.getUChar("alarmVol", 150);
    MenuController::setAlarmVolumeSilent(alarmVol);

    uint8_t brightness = prefs.getUChar("brightness", 128);
    MenuController::setBrightness(brightness);

    bool research   = prefs.getBool("research",  false);
    bool wifi       = prefs.getBool("wifi",      false);
    bool wardriving = prefs.getBool("wardrive",  false);

    uint16_t logMask = prefs.getUShort("logMask", logGetEnabledCategories());  // ← VOR prefs.end()

    prefs.end();

    MenuController::setResearchMode(research);
    MenuController::setWifiEnabled(wifi);
    MenuController::setWardriving(wardriving);
    logSetEnabledCategories(logMask);   // ← Setzen bleibt nach prefs.end(), das ist okay

    LOG(LOG_SYSTEM, "Menu settings loaded");
}

void MenuSettings::save() {
    prefs.begin(NS, false);

    prefs.putBool("audioEn",   MenuController::getAudioEnabled());
    prefs.putBool("audioSusp", MenuController::getAudioSuspicious());
    prefs.putBool("audioFlock",MenuController::getAudioFlock());
    prefs.putBool("audioDrone",MenuController::getAudioDrone());
    prefs.putBool("audioFlip", MenuController::getAudioFlipper());
    prefs.putBool("audioPwn",  MenuController::getAudioPwnBeacon());
    prefs.putUChar("brightness", MenuController::getBrightness());
    prefs.putUChar("alarmVol", MenuController::getAlarmVolume());

    prefs.putBool("research",  MenuController::getResearchMode());
    prefs.putBool("wifi",      MenuController::getWifiEnabled());
    prefs.putBool("wardrive",  MenuController::getWardriving());

    prefs.putUShort("logMask", logGetEnabledCategories());

    prefs.end();
    LOG(LOG_CONTROL, "Menu settings saved to NVS");
}
