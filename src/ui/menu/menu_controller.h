#ifndef MENU_CONTROLLER_H
#define MENU_CONTROLLER_H

#include <Arduino.h>

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

// Open / close the menu overlay
void open();
void close();
bool isOpen();

// Navigation — call from loop() key handlers
void navigateUp();
void navigateDown();
void adjustLeft();
void adjustRight();
void selectCurrent();   // toggle or execute current item

// Draw — call when menu is open and state changes
void draw();

// Returns current state pointer (read by ble_scanner, GhostBLE.ino etc.)
MenuState* getState();

} // namespace MenuController

#endif // MENU_CONTROLLER_H
