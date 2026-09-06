// sus_device_view.cpp
#include "sus_device_view.h"
#include <M5Unified.h>
#include <cstring>

#include "app/context/sus_log_context.h"

#include "ui/finder/approach_view.h"
#include "ui/menu/menu_controller.h"

#include "core/findmy/findmy_sound.h"

#include "infrastructure/platform/hardware_config.h"
#include "infrastructure/logging/logger.h"


namespace SusDeviceView {

// ── Layout constants ──────────────────────────────────────────
static constexpr int MENU_X        = 0;
static constexpr int MENU_Y        = 0;
static constexpr int MENU_W        = 240;
static constexpr int MENU_H        = 135;
static constexpr int ROW_H         = 11;    // pixels per row
static constexpr int ROWS_VISIBLE  = 11;    // rows on screen at once
static constexpr int INDENT_W      = 8;     // sub-item indent pixels

// ── Colors (RGB565) ───────────────────────────────────────────
static constexpr uint16_t COL_CURSOR    = 0x07E0;   // green
static constexpr uint16_t COL_STATUSBAR = 0x5ACB;   // very dark for status bar

static bool menuOpen_ = false;
static int  cursorIdx_ = 0;

// Find My action popup
// 0 = SEARCH
// 1 = SOUND
// 2 = CANCEL
static bool actionPopupOpen_ = false;
static int  actionCursor_ = 0;
static int  actionDeviceIdx_ = -1;

static String timeAgo(uint32_t ts) {
    uint32_t diff = (millis() - ts) / 1000;
    if (diff < 60) return String(diff) + "s ago";
    if (diff < 3600) return String(diff / 60) + "m ago";
    return String(diff / 3600) + "h ago";
}

void open() {
    menuOpen_ = true;
    cursorIdx_ = 0;
    draw();
}

void close() {
    menuOpen_ = false;
    MenuController::open();
}

bool isOpen() { return menuOpen_; }

void navigateNext() {
    if (!menuOpen_) return;

    if (actionPopupOpen_) {
        actionCursor_ = (actionCursor_ + 1) % 3;
        draw();
        return;
    }

    int total = SusLog::count();
    if (total == 0) return;

    cursorIdx_ = (cursorIdx_ + 1) % total;
    draw();
}

void navigatePrev() {
    if (!menuOpen_) return;

    if (actionPopupOpen_) {
        actionCursor_ = (actionCursor_ - 1 + 3) % 3;
        draw();
        return;
    }

    int total = SusLog::count();
    if (total == 0) return;

    cursorIdx_ = (cursorIdx_ - 1 + total) % total;
    draw();
}

void selectCurrent() {
    if (!menuOpen_) return;
    // ============================================================
    // FIND MY ACTION POPUP
    // ============================================================
    if (actionPopupOpen_) {

        if (actionDeviceIdx_ < 0 ||
            actionDeviceIdx_ >= SusLog::count()) {

            actionPopupOpen_ = false;
            actionDeviceIdx_ = -1;
            draw();
            return;
        }

        const auto& e = SusLog::get(actionDeviceIdx_);

        switch (actionCursor_) {
            // ----------------------------------------------------
            // 0 = SEARCH
            // ----------------------------------------------------
            case 0: {
                actionPopupOpen_ = false;
                actionDeviceIdx_ = -1;

                close();

                ApproachView::open(e.mac, e.label, ApproachView::ReturnTarget::SusList);
                break;
            }
            // ----------------------------------------------------
            // 1 = SOUND
            // ----------------------------------------------------
            case 1:
            {
                actionPopupOpen_ = false;
                actionDeviceIdx_ = -1;

                LOG(LOG_TARGET, String("Sus Devices - Find My Sound: ") + e.mac);

                M5.Lcd.fillScreen(0x0020);
                M5.Lcd.setTextSize(1);

                M5.Lcd.setTextColor(COL_CURSOR, 0x0020);
                M5.Lcd.setCursor(4, 8);
                M5.Lcd.print("AIR TAG SOUND CHECK");

                M5.Lcd.setTextColor(0x8C71, 0x0020);
                M5.Lcd.setCursor(4, 28);
                M5.Lcd.print("Tracker:");

                M5.Lcd.setCursor(4, 40);
                M5.Lcd.print(e.mac);

                M5.Lcd.setCursor(4, 65);
                M5.Lcd.print("Connecting...");

                M5.Lcd.setCursor(4, 90);
                M5.Lcd.print("Please wait");

                M5.Lcd.setCursor(4, 120);
                M5.Lcd.print("BLE operation");

                M5.Lcd.setTextColor(COL_CURSOR, 0x0020);
                M5.Lcd.setCursor(220, 120);
                M5.Lcd.print("_");

                M5.Lcd.display();

                FindMySound::Result result = FindMySound::play(e.mac);

                LOG(LOG_TARGET, String("Find My Sound result: ") + FindMySound::resultToString(result));

                M5.Lcd.setCursor(4, 90);
                M5.Lcd.fillRect(0, 88, 240, 12, 0x0020);

                M5.Lcd.setTextColor(COL_CURSOR, 0x0020);
                M5.Lcd.print(FindMySound::resultToString(result));

                delay(1500);

                draw();
                break;
            }
            // ----------------------------------------------------
            // 2 = CANCEL
            // ----------------------------------------------------
            case 2: {
                actionPopupOpen_ = false;
                actionDeviceIdx_ = -1;

                draw();
                break;
            }
        }
        return;
    }
    // ============================================================
    // NORMAL DEVICE SELECTION
    // ============================================================
    int total = SusLog::count();

    if (total == 0) return;

    const auto& e = SusLog::get(cursorIdx_);

    // Find My tracker -> show action popup
    if (strcmp(e.label, "Find My Tracker") == 0) {

        actionPopupOpen_ = true;
        actionCursor_ = 0;
        actionDeviceIdx_ = cursorIdx_;

        draw();

        return;
    }

    // All other suspicious devices -> Approach View
    close();

    ApproachView::open(e.mac, e.label, ApproachView::ReturnTarget::SusList);
}

static void drawActionPopup()
{
    constexpr uint16_t BG     = 0x0020;
    constexpr uint16_t GREEN  = 0x07E0;
    constexpr uint16_t GREY   = 0x8C71;
    constexpr uint16_t CURSOR = 0x0341;

    M5.Lcd.fillScreen(BG);
    M5.Lcd.setTextSize(1);

    // Header
    M5.Lcd.setTextColor(GREEN, BG);
    M5.Lcd.setCursor(4, 4);
    M5.Lcd.print("FIND MY TRACKER");

    // MAC address
    if (actionDeviceIdx_ >= 0 &&
        actionDeviceIdx_ < SusLog::count()) {

        const auto& e = SusLog::get(actionDeviceIdx_);

        M5.Lcd.setTextColor(GREY, BG);
        M5.Lcd.setCursor(4, 16);
        M5.Lcd.print(e.mac);
    }

    const char* options[] = {
        "SEARCH",
        "SOUND",
        "CANCEL"
    };

    constexpr int START_Y = 40;
    constexpr int ACTION_H = 22;

    for (int i = 0; i < 3; ++i) {

        const bool selected = (i == actionCursor_);

        const uint16_t bg = selected ? CURSOR : BG;
        const uint16_t fg = selected ? GREEN : GREY;

        M5.Lcd.fillRect(0, START_Y + i * ACTION_H, MENU_W, ACTION_H, bg);
        M5.Lcd.setTextColor(fg, bg);
        M5.Lcd.setCursor(12, START_Y + i * ACTION_H + 7);
        M5.Lcd.printf("%c %s", selected ? '>' : ' ', options[i]);
    }

    // Status bar
    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);

#if HAS_KEYBOARD
    M5.Lcd.print("^:up  v:down  ok:select  esc:back");
#else
    M5.Lcd.print("blue:next  big:select  hold big:back");
#endif
}

void draw() {
    if (!menuOpen_) return;

    if (actionPopupOpen_) {
        drawActionPopup();
        return;
    }

    M5.Lcd.fillScreen(0x0020);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x07E0, 0x0020);
    M5.Lcd.setCursor(4, 2);
    M5.Lcd.printf("SUS DEVICES (%d)", SusLog::count());

    if (SusLog::count() == 0) {
        M5.Lcd.setTextColor(0x8C71, 0x0020);
        M5.Lcd.setCursor(4, 20);
        M5.Lcd.print("No suspicious devices yet");
        // Status bar at bottom
        M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
        M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
        M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
    #if HAS_KEYBOARD
        M5.Lcd.print("^:up  v:down  ok:select  esc:close");
    #else
        M5.Lcd.print("blue:next  big:select  hold big:back");
    #endif
        return;
    }

    int total = SusLog::count();
    int shown = min(total, 5);
    int y = 16;

    for (int i = 0; i < shown; i++) {
        int idx = (cursorIdx_ + i) % total;
        const SusLog::Entry& e = SusLog::get(idx);

        bool selected = (i == 0);
        uint16_t bg = selected ? 0x0341 : 0x0020;
        M5.Lcd.fillRect(0, y, 240, 22, bg);

        M5.Lcd.setTextColor(0x07E0, bg);
        M5.Lcd.setCursor(4, y + 2);
        M5.Lcd.print(e.label);

        M5.Lcd.setTextColor(0x8C71, bg);
        M5.Lcd.setCursor(4, y + 12);
        M5.Lcd.printf("%s  %ddBm  %s", e.mac, e.rssi, timeAgo(e.timestamp).c_str());

        y += 22;
    }

    // Status bar at bottom
    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
#if HAS_KEYBOARD
    M5.Lcd.print("^:up  v:down  ok:select  esc:close");
#else
    M5.Lcd.print("blue:next  big:select  hold big:back");
#endif
}

} // namespace SusDeviceView
