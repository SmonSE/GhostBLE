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

    uint8_t brightness = prefs.getUChar("brightness", 128);
    MenuController::setBrightness(brightness);

    bool research   = prefs.getBool("research",  false);
    bool wifi       = prefs.getBool("wifi",      false);
    bool wardriving = prefs.getBool("wardrive",  false);

    prefs.end();

    // Seiteneffekt-behaftete Toggles NACH prefs.end(), falls die
    // aufgerufenen Funktionen selbst NVS/SD zugreifen (Konfliktvermeidung)
    MenuController::setResearchMode(research);
    MenuController::setWifiEnabled(wifi);
    MenuController::setWardriving(wardriving);

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

    prefs.putBool("research",  MenuController::getResearchMode());
    prefs.putBool("wifi",      MenuController::getWifiEnabled());
    prefs.putBool("wardrive",  MenuController::getWardriving());

    prefs.end();
    LOG(LOG_CONTROL, "Menu settings saved to NVS");
}