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

#include "assets/nibblesFront.h"
#include "assets/nibblesHappy.h"


namespace FinderListView {

static bool open_ = false;
static int  cursorIdx_ = 0;

void open() {
    open_ = true;
    cursorIdx_ = 0;
    draw();
}

void close() { 
    open_ = false; 

    M5.Lcd.fillScreen(0x00C4);
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
    showScanIcon();
    drawXPBar(LEVEL_TEXT_X, BOTTOM_BAR_Y, true);
}

bool isOpen() { return open_; }

void refresh() {
    if (!open_) return;

    //LOG(LOG_CONTROL, "Finder — refreshing scan");
    drawScanning();

    bool wasScanning = ScanContext::bleScanEnabled.load();
    if (wasScanning) {
        ScanContext::bleScanEnabled.store(false);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

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

void selectCurrent() {
    if (!open_) return;
    int total = DeviceFinder::count();
    if (total == 0) return;

    const auto& d = DeviceFinder::get(cursorIdx_);
    //close();
    ApproachView::open(d.mac, d.name);
}

void draw() {
    if (!open_) return;

    M5.Lcd.fillScreen(0x0020);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x07E0, 0x0020);
    M5.Lcd.setCursor(4, 2);
    M5.Lcd.printf("FOUND DEVICES (%d)", DeviceFinder::count());

    if (DeviceFinder::count() == 0) {
        M5.Lcd.setTextColor(0x8C71, 0x0020);
        M5.Lcd.setCursor(4, 20);
        M5.Lcd.print("No devices found");
        M5.Lcd.setCursor(4, 125);
        M5.Lcd.print("c: close");
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

    M5.Lcd.setTextColor(0x2945, 0x0020);
    M5.Lcd.setCursor(4, 125);
    M5.Lcd.print("v:next  >:select  c:close");
    //M5.Lcd.print("BtnA:next  BtnB:select  hold:close");
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

    // Terminal-Zeilen nacheinander einblenden
    typeText(8, 32, "> BLE Adapter........ ", GREEN, BG, 4);
    typeText(M5.Lcd.getCursorX(), 32, "OK", CYAN, BG, 30);
    delay(000);

    typeText(8, 46, "> Privacy Engine..... ", GREEN, BG, 4);
    typeText(M5.Lcd.getCursorX(), 46, "READY", CYAN, BG, 30);
    delay(000);

    typeText(8, 60, "> Device Scan........ ", GREEN, BG, 4);
    typeText(M5.Lcd.getCursorX(), 60, "RUNNING", CYAN, BG, 30);
    delay(1000);

    M5.Lcd.setTextColor(GREEN, BG);
    M5.Lcd.setCursor(8, 86);
    M5.Lcd.print("> Searching nearby devices");

    M5.Lcd.setTextColor(GREY, BG);
    M5.Lcd.setCursor(8, 102);
    M5.Lcd.print("> Duration: ~10 seconds");

    /*
    // Animierter Spinner + Punkte-Ellipse, ca. 1.5s
    const char* frames[] = {"|", "/", "-", "\\"};
    for (int cycle = 0; cycle < 60; cycle++) {
        M5.Lcd.fillRect(8, 112, 220, 12, BG);   // Spinner-Zeile löschen

        M5.Lcd.setTextColor(GREEN, BG);
        M5.Lcd.setCursor(8, 112);
        M5.Lcd.print(frames[cycle % 4]);

        M5.Lcd.setCursor(20, 112);
        int dots = (cycle % 4) + 1;
        for (int d = 0; d < dots; d++) M5.Lcd.print(".");

        delay(150);
    }

    // Blinkender Cursor am Ende
    for (int i = 0; i < 3; i++) {
        M5.Lcd.setTextColor(GREEN, BG);
        M5.Lcd.setCursor(8, 122);
        M5.Lcd.print("_");
        delay(200);
        M5.Lcd.fillRect(8, 122, 8, 10, BG);
        delay(200);
    }
    */

    M5.Lcd.setTextColor(GREEN, BG);
    M5.Lcd.setCursor(8, 122);
    M5.Lcd.print("_");
}

} // namespace FinderListView
