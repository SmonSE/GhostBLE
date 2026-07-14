#ifndef MENU_CONTROLLER_H
#define MENU_CONTROLLER_H

#include <Arduino.h>
#include "config/menu_settings.h"

// ============================================================
//  GhostBLE Menu Controller
//
//  Renders a scrollable menu on the M5 display with checkbox
//  items (filled = ON, empty = OFF) and section headers.
//
//  Navigation:
//    Cardputer:  arrow keys up/down, ENTER to toggle, FN to close
//    StickS3:    BtnA = down, BtnB = toggle, long-press M5 = open/close
//
//  All toggleable state lives in MenuState — one source of truth
//  for every on/off feature in the firmware.
// ============================================================

// ── Menu item types ───────────────────────────────────────────
enum class MenuItemType {
    Toggle,     // checkbox ON/OFF
    Action,     // execute immediately (no checkbox)
    Section,    // non-selectable header row
    ValueInfo,  // read-only label + value (e.g. "LV12")
    Spacer,     // empty row for spacing
    Slider      // interactive slider
};

// ── All toggleable state in one struct ───────────────────────
struct MenuState {

    // AUDIO (master + per-category)
    bool audioEnabled      = true;
    bool audioSuspicious   = true;
    bool audioFlock        = true;
    bool audioDrone        = false;
    bool audioFlipper      = true;
    bool audioPwnBeacon    = true;
    bool audioEvilMode     = false;
};

// ── Menu controller ───────────────────────────────────────────
namespace MenuController {

// Initialise — call once in setup()
void init(MenuState* state);
MenuState* getState();

bool getAudioEnabled();
void setAudioEnabled(bool v);

bool getAudioSuspicious();
void setAudioSuspicious(bool v);

bool getAudioFlock();
void setAudioFlock(bool v);

bool getAudioDrone();
void setAudioDrone(bool v);

bool getAudioFlipper();
void setAudioFlipper(bool v);

bool getAudioPwnBeacon();
void setAudioPwnBeacon(bool v);

bool getResearchMode();
void setResearchMode(bool v);

bool getWifiEnabled();
void setWifiEnabled(bool v);

bool getWardriving();
void setWardriving(bool v);

uint8_t getAlarmVolume();
void    setAlarmVolume(uint8_t val);
void    setAlarmVolumeSilent(uint8_t val);

uint8_t getBrightness();
void    setBrightness(uint8_t val);

// Open / close the menu overlay
void open();
void close();
void closeSilent();
bool isOpen();

// Navigation — call from loop() key handlers
void navigateUp();
void navigateDown();
void adjustLeft();
void adjustRight();
void selectCurrent();   // toggle or execute current item

uint8_t getBrightness();
void    setBrightness(uint8_t val);

// Draw — call when menu is open and state changes
void draw();

} // namespace MenuController

#endif // MENU_CONTROLLER_H
