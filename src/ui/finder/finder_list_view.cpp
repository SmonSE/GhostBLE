// finder_list_view.cpp
#include "finder_list_view.h"
#include <M5Unified.h>

#include "app/context/device_finder.h"
#include "app/context/scan_context.h"

#include "config/ui_config.h"

#include "ui/finder/approach_view.h"
#include "ui/overlay/draw_overlay.h"
#include "ui/expression/show_expression.h"
#include "ui/icons/scan_icon.h"
#include "ui/menu/menu_controller.h"

#include "assets/nibblesFront.h"
#include "assets/nibblesHappy.h"

#include "infrastructure/platform/hardware_config.h"
#include "infrastructure/ble/ble_scanner.h"


namespace FinderListView {

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

static bool open_ = false;
static int  cursorIdx_ = 0;

void open() {
    if (ScanContext::bleScanEnabled.load()) {
        LOG(LOG_CONTROL, "Approach View — stopping main scan");
        stopBleScan();
    }
    
    open_ = true;
    cursorIdx_ = 0;
    draw();
}

void close() { 
    open_ = false; 

    MenuController::open();
}

bool isOpen() { return open_; }

void refresh() {
    if (!open_) return;

    LOG(LOG_CONTROL, "Finder — refreshing scan");
    drawScanning();

    DeviceFinder::scan5s();

    cursorIdx_ = 0;
    draw();
}

void navigateNext() {
    if (!open_) return;
    int total = DeviceFinder::count();
    if (total == 0) return;
    cursorIdx_ = (cursorIdx_ + 1) % total;
    draw();
}

void navigatePrev() {
    if (!open_) return;
    int total = DeviceFinder::count();
    if (total == 0) return;
    cursorIdx_ = (cursorIdx_ - 1 + total) % total;
    draw();
}

void selectCurrent() {
    if (!open_) return;
    int total = DeviceFinder::count();
    if (total == 0) return;

    const auto& d = DeviceFinder::get(cursorIdx_);
    close();
    ApproachView::open(d.mac, d.name, ApproachView::ReturnTarget::FinderList);
}

void draw() {
    if (!open_) return;

    M5.Lcd.fillScreen(0x0020);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x07E0, 0x0020);
    M5.Lcd.setCursor(4, 2);
    M5.Lcd.printf("FOUND DEVICES (%d)", DeviceFinder::count());

    // Status bar at bottom
    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);

    if (DeviceFinder::count() == 0) {
        M5.Lcd.print("No devices found");
    #if HAS_KEYBOARD
        M5.Lcd.print(" f:refresh  esc:close");
    #else
        M5.Lcd.print("blue:refresh  hold big:back");
    #endif
        return;    
    }

    int total = DeviceFinder::count();
    int shown = min(total, 5);
    int y = 16;

    for (int i = 0; i < shown; i++) {
        int idx = (cursorIdx_ + i) % total;
        const auto& d = DeviceFinder::get(idx);

        bool selected = (i == 0);
        uint16_t bg = selected ? 0x0341 : 0x0020;
        M5.Lcd.fillRect(0, y, 240, 22, bg);

        M5.Lcd.setTextColor(0x07E0, bg);
        M5.Lcd.setCursor(4, y + 2);
        M5.Lcd.print(d.name);

        M5.Lcd.setTextColor(0x8C71, bg);
        M5.Lcd.setCursor(4, y + 12);
        M5.Lcd.printf("%s  %ddBm", d.mac, d.rssi);

        y += 22;
    }

    // Status bar at bottom
    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
#if HAS_KEYBOARD
    M5.Lcd.print(" ^:up  v:down  ok:select  esc:close");
#else
    M5.Lcd.print("blue:next  big:select  hold big:back");
#endif
}

static void typeText(int x, int y, const char* text, uint16_t color, uint16_t bg, int delayMs = 8) {
    M5.Lcd.setTextColor(color, bg);
    M5.Lcd.setCursor(x, y);
    for (const char* p = text; *p; p++) {
        M5.Lcd.print(*p);
        delay(delayMs);
    }
}

void drawScanning() {
    constexpr uint16_t BG    = 0x0020;
    constexpr uint16_t GREEN = 0x07E0;
    constexpr uint16_t CYAN  = 0x03EF;
    constexpr uint16_t GREY  = 0x8410;

    M5.Lcd.fillScreen(BG);
    M5.Lcd.setTextSize(1);

    // Header mit Typewriter-Effekt
    typeText(6, 6, "> GhostBLE", GREEN, BG, 15);
    delay(100);

    M5.Lcd.drawFastHLine(4, 18, 232, GREEN);
    delay(500);

    // Terminal rows with typewriter effect
    typeText(8, 32, "> BLE Adapter........ ", GREEN, BG, 4);
    typeText(M5.Lcd.getCursorX(), 32, "OK", CYAN, BG, 30);
    delay(100);

    typeText(8, 46, "> Privacy Engine..... ", GREEN, BG, 4);
    typeText(M5.Lcd.getCursorX(), 46, "READY", CYAN, BG, 30);
    delay(100);

    typeText(8, 60, "> Device Scan........ ", GREEN, BG, 4);
    typeText(M5.Lcd.getCursorX(), 60, "RUNNING", CYAN, BG, 30);
    delay(500);

    M5.Lcd.setTextColor(GREEN, BG);
    M5.Lcd.setCursor(8, 86);
    M5.Lcd.print("> Searching nearby devices");

    M5.Lcd.setTextColor(GREY, BG);
    M5.Lcd.setCursor(8, 102);
    M5.Lcd.print("> Duration: ~10 seconds");

    M5.Lcd.setTextColor(GREEN, BG);
    M5.Lcd.setCursor(8, 122);
    M5.Lcd.print("_");
}

} // namespace FinderListView
