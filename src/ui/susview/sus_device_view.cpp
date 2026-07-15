// sus_device_view.cpp
#include "sus_device_view.h"
#include <M5Unified.h>
#include "app/context/sus_log_context.h"

namespace SusDeviceView {

static bool menuOpen_ = false;
static int  cursorIdx_ = 0;

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
}

bool isOpen() { return menuOpen_; }

void navigateNext() {
    if (!menuOpen_) return;
    int total = SusLog::count();
    if (total == 0) return;
    cursorIdx_ = (cursorIdx_ + 1) % total;
    draw();
}

void draw() {
    if (!menuOpen_) return;

    M5.Lcd.fillScreen(0x0020);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x07E0, 0x0020);
    M5.Lcd.setCursor(4, 2);
    M5.Lcd.printf("SUS DEVICES (%d)", SusLog::count());

    if (SusLog::count() == 0) {
        M5.Lcd.setTextColor(0x8C71, 0x0020);
        M5.Lcd.setCursor(4, 20);
        M5.Lcd.print("No suspicious devices yet");
        M5.Lcd.setCursor(4, 125);
        M5.Lcd.print("M: close");
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

    M5.Lcd.setTextColor(0x2945, 0x0020);
    M5.Lcd.setCursor(4, 125);
    M5.Lcd.print("v:next  M:close");
}

} // namespace SusDeviceView
