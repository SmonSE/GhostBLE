#include "connected_device_view.h"
#include <M5Unified.h>
#include <cstring>

#include "app/context/connected_device_context.h"
#include "ui/menu/menu_controller.h"
#include "infrastructure/logging/logger.h"
#include "infrastructure/platform/hardware_config.h"
#include "infrastructure/logging/gatt_logger.h"
#include "app/context/scan_context.h"
#include "infrastructure/ble/ble_scanner.h"


namespace ConnectedDeviceView {

static constexpr int MENU_W = 240;
static constexpr int MENU_H = 135;
static constexpr int ROW_H  = 11;

static constexpr uint16_t COL_CURSOR    = 0x07E0;
static constexpr uint16_t COL_STATUSBAR = 0x5ACB;

static bool menuOpen_  = false;
static int  cursorIdx_ = 0;

// Popup: 0 = START LOG / STOP LOG, 1 = CANCEL
static bool actionPopupOpen_  = false;
static int  actionCursor_     = 0;
static int  actionDeviceIdx_  = -1;

static bool sessionScreenOpen_    = false;
static bool stopRequestedByUser_  = false;

static String timeAgo(uint32_t ts) {
    uint32_t diff = (millis() - ts) / 1000;
    if (diff < 60) return String(diff) + "s ago";
    if (diff < 3600) return String(diff / 60) + "m ago";
    return String(diff / 3600) + "h ago";
}

void open()  { menuOpen_ = true; cursorIdx_ = 0; draw(); }
void close() { menuOpen_ = false; MenuController::open(); }
bool isOpen() { return menuOpen_; }

void navigateNext() {
    if (!menuOpen_ || sessionScreenOpen_) return;
    if (actionPopupOpen_) { actionCursor_ = (actionCursor_ + 1) % 2; draw(); return; }

    int total = ConnectedLog::count();
    if (total == 0) return;
    cursorIdx_ = (cursorIdx_ + 1) % total;
    draw();
}

void navigatePrev() {
    if (!menuOpen_ || sessionScreenOpen_) return;
    if (actionPopupOpen_) { actionCursor_ = (actionCursor_ - 1 + 2) % 2; draw(); return; }

    int total = ConnectedLog::count();
    if (total == 0) return;
    cursorIdx_ = (cursorIdx_ - 1 + total) % total;
    draw();
}

void selectCurrent() {
    if (!menuOpen_) return;

    // ============================================================
    // SESSION SCREEN: Select = Stop Logging
    // ============================================================
    if (sessionScreenOpen_) {
        if (!stopRequestedByUser_) {
            stopRequestedByUser_ = true;
            GattLogger::stopSession();
            draw();
        }
        return;
    }

    // ============================================================
    // ACTION POPUP: Logging starten/stoppen
    // ============================================================
    if (actionPopupOpen_) {
        if (actionDeviceIdx_ < 0 || actionDeviceIdx_ >= ConnectedLog::count()) {
            actionPopupOpen_ = false;
            actionDeviceIdx_ = -1;
            draw();
            return;
        }

        const auto& e = ConnectedLog::get(actionDeviceIdx_);

        switch (actionCursor_) {
            case 0: {
                if (e.isLogging) {
                    LOG(LOG_GATT, String("GATT Logging gestoppt: ") + e.mac);
                    GattLogger::stopSession();
                } else {
                    LOG(LOG_GATT, String("GATT Logging gestartet: ") + e.mac);

                    if (ScanContext::bleScanEnabled.load()) {
                        LOG(LOG_CONTROL, "Connected Device View — stoppe Hauptscan für GATT-Log");
                        stopBleScan();
                    }

                    GattLogger::startSession(e.mac, e.label, e.addrType);

                    sessionScreenOpen_   = true;   // NEU
                    stopRequestedByUser_ = false;  // NEU
                }
                actionPopupOpen_ = false;
                actionDeviceIdx_ = -1;
                draw();
                break;
            }
            case 1: { // CANCEL
                actionPopupOpen_ = false;
                actionDeviceIdx_ = -1;
                draw();
                break;
            }
        }
        return;
    }

    // ============================================================
    // NORMALE AUSWAHL -> Popup öffnen
    // ============================================================
    int total = ConnectedLog::count();
    if (total == 0) return;

    actionPopupOpen_ = true;
    actionCursor_    = 0;
    actionDeviceIdx_ = cursorIdx_;
    draw();
}

static void drawActionPopup() {
    constexpr uint16_t BG     = 0x0020;
    constexpr uint16_t GREEN  = 0x07E0;
    constexpr uint16_t GREY   = 0x8C71;
    constexpr uint16_t CURSOR = 0x0341;

    M5.Lcd.fillScreen(BG);
    M5.Lcd.setTextSize(1);

    M5.Lcd.setTextColor(GREEN, BG);
    M5.Lcd.setCursor(4, 4);
    M5.Lcd.print("CONNECTABLE DEVICE");

    bool isLoggingNow = false;
    if (actionDeviceIdx_ >= 0 && actionDeviceIdx_ < ConnectedLog::count()) {
        const auto& e = ConnectedLog::get(actionDeviceIdx_);
        isLoggingNow = e.isLogging;

        M5.Lcd.setTextColor(GREY, BG);
        M5.Lcd.setCursor(4, 16);
        M5.Lcd.print(e.mac);
    }

    const char* options[] = { isLoggingNow ? "STOP LOG" : "START LOG", "CANCEL" };
    constexpr int START_Y = 40;
    constexpr int ACTION_H = 22;

    for (int i = 0; i < 2; ++i) {
        const bool selected = (i == actionCursor_);
        const uint16_t bg = selected ? CURSOR : BG;
        const uint16_t fg = selected ? GREEN : GREY;

        M5.Lcd.fillRect(0, START_Y + i * ACTION_H, MENU_W, ACTION_H, bg);
        M5.Lcd.setTextColor(fg, bg);
        M5.Lcd.setCursor(12, START_Y + i * ACTION_H + 7);
        M5.Lcd.printf("%c %s", selected ? '>' : ' ', options[i]);
    }

    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
#if HAS_KEYBOARD
    M5.Lcd.print("^:up  v:down  ok:select  esc:back");
#else
    M5.Lcd.print("blue:next  big:select  hold big:back");
#endif
}

static void drawSessionScreenStatic(const GattLogger::SessionInfo& info) {
    constexpr uint16_t BG    = 0x0020;
    constexpr uint16_t GREEN = 0x07E0;
    constexpr uint16_t GREY  = 0x8C71;

    M5.Lcd.fillScreen(BG);   // nur EINMAL beim Öffnen
    M5.Lcd.setTextSize(1);

    M5.Lcd.setTextColor(GREEN, BG);
    M5.Lcd.setCursor(4, 4);
    M5.Lcd.print("DEVICE LOGGING");

    M5.Lcd.setTextColor(GREY, BG);
    M5.Lcd.setCursor(4, 16);
    M5.Lcd.print(info.label.c_str());
    M5.Lcd.setCursor(4, 26);
    M5.Lcd.print(info.mac.c_str());

    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
#if HAS_KEYBOARD
    M5.Lcd.print("ok:stop log");
#else
    M5.Lcd.print("big:stop log");
#endif
}

static void drawSessionScreenDynamic(const GattLogger::SessionInfo& info) {
    constexpr uint16_t BG    = 0x0020;
    constexpr uint16_t GREEN = 0x07E0;
    constexpr uint16_t GREY  = 0x8C71;
    constexpr uint16_t RED   = 0xF800;

    uint32_t totalSec = info.elapsedMs / 1000;
    uint32_t mm = totalSec / 60;
    uint32_t ss = totalSec % 60;

    // Nur den Timer-Bereich löschen, nicht den ganzen Screen
    M5.Lcd.fillRect(0, 40, 120, 12, BG);
    M5.Lcd.setTextColor(GREEN, BG);
    M5.Lcd.setCursor(4, 42);
    M5.Lcd.printf("Zeit: %02lu:%02lu", (unsigned long)mm, (unsigned long)ss);

    M5.Lcd.fillRect(125, 40, 115, 12, BG);
    M5.Lcd.setTextColor(info.connected ? GREEN : RED, BG);
    M5.Lcd.setCursor(130, 42);
    M5.Lcd.print(info.connected ? "CONNECTED" : "DISCONNECTED");

    M5.Lcd.fillRect(0, 56, 240, 34, BG);
    M5.Lcd.setTextColor(GREY, BG);
    M5.Lcd.setCursor(4, 58);
    M5.Lcd.printf("Services: %u  Chars: %u", info.serviceCount, info.charCount);
    M5.Lcd.setCursor(4, 68);
    M5.Lcd.printf("Notify/Indicate: %lu", (unsigned long)info.notifyCount);
    M5.Lcd.setCursor(4, 78);
    M5.Lcd.printf("Changed (poll):  %lu", (unsigned long)info.changedCount);

    M5.Lcd.fillRect(0, 92, 240, 20, BG);

    if (!info.lastValueDecoded.empty()) {
        // UUID kürzen, damit sie auf den 240px-Screen passt (Cardputer)
        String uuidShort = info.lastValueUuid.c_str();
        if (uuidShort.length() > 8) uuidShort = uuidShort.substring(0, 8) + "...";

        uint32_t ageSec = (millis() - info.lastValueMs) / 1000;

        M5.Lcd.setTextColor(GREEN, BG);
        M5.Lcd.setCursor(4, 94);
        M5.Lcd.printf("Last: %s (%lus ago)", uuidShort.c_str(), (unsigned long)ageSec);

        M5.Lcd.setTextColor(GREY, BG);
        M5.Lcd.setCursor(4, 104);
        M5.Lcd.print(info.lastValueDecoded.c_str());
    }
}

void draw() {
    if (!menuOpen_) return;

    if (sessionScreenOpen_) {
        auto info = GattLogger::getSessionInfo();
        drawSessionScreenStatic(info);    // voller Redraw NUR hier
        drawSessionScreenDynamic(info);
        return;
    }
    if (actionPopupOpen_) { drawActionPopup(); return; }

    M5.Lcd.fillScreen(0x0020);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x07E0, 0x0020);
    M5.Lcd.setCursor(4, 2);
    M5.Lcd.printf("CONNECTABLE DEVICES (%d)", ConnectedLog::count());

    if (ConnectedLog::count() == 0) {
        M5.Lcd.setTextColor(0x8C71, 0x0020);
        M5.Lcd.setCursor(4, 20);
        M5.Lcd.print("No connected devices");

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

    int total = ConnectedLog::count();
    int shown = min(total, 5);
    int y = 16;

    for (int i = 0; i < shown; i++) {
        int idx = (cursorIdx_ + i) % total;
        const ConnectedLog::Entry& e = ConnectedLog::get(idx);

        bool selected = (i == 0);
        uint16_t bg = selected ? 0x0341 : 0x0020;
        M5.Lcd.fillRect(0, y, 240, 22, bg);

        M5.Lcd.setTextColor(0x07E0, bg);
        M5.Lcd.setCursor(4, y + 2);
        M5.Lcd.printf("%s%s", e.label, e.isLogging ? " [LOG]" : "");

        M5.Lcd.setTextColor(0x8C71, bg);
        M5.Lcd.setCursor(4, y + 12);
        M5.Lcd.printf("%s  %ddBm  %s", e.mac, e.rssi, timeAgo(e.timestamp).c_str());

        y += 22;
    }

    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
#if HAS_KEYBOARD
    M5.Lcd.print("^:up  v:down  ok:close");
#else
    M5.Lcd.print("blue:next  big:select  hold big:back");
#endif
}

void tick() {
    if (!menuOpen_ || !sessionScreenOpen_) return;

    if (!GattLogger::isSessionActive()) {
        sessionScreenOpen_   = false;
        stopRequestedByUser_ = false;
        draw();
        return;
    }

    auto info = GattLogger::getSessionInfo();
    drawSessionScreenDynamic(info);
}

} // namespace ConnectedDeviceView
