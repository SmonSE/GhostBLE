// approach_view.cpp
#include "approach_view.h"
#include <NimBLEDevice.h>
#include <M5Unified.h>

#include "ui/menu/menu_controller.h"
#include "ui/finder/finder_list_view.h"
#include "ui/susview/sus_device_view.h"

#include "infrastructure/platform/hardware_config.h"
#include "infrastructure/ble/ble_scanner.h"


namespace ApproachView {

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

// ── State ────────────────────────────────────────────────────
static bool    open_          = false;
static bool    staticDrawn_   = false;
static bool    scanStopped_   = false;
static String  targetMac_;
static String  targetName_;
static int8_t  smoothedRssi_  = -100;
static int8_t  lastDrawnRssi_ = -127;
static bool    lastFound_     = false;
static unsigned long lastScanTime_ = 0;
static unsigned long lastBeepTime_ = 0;

// ── Colors (RGB565) ─────────────────────────────────────────
static constexpr uint16_t COL_SCREEN_BG = 0x0020;
static constexpr uint16_t COL_TEXT_DIM  = 0x2945;
static constexpr uint16_t COL_BAR_BG    = 0x0120;
static constexpr uint16_t COL_BAR_BORDER = 0x2945;
static constexpr uint16_t COL_RED       = 0xF800;
static constexpr uint16_t COL_YELLOW    = 0xFFE0;
static constexpr uint16_t COL_GREEN     = 0x07E0;

// ── Bar geometry ─────────────────────────────────────────────
static constexpr int BAR_X = 10;
static constexpr int BAR_Y = 55;
static constexpr int BAR_W = 220;
static constexpr int BAR_H = 26;

// ============================================================
//  Helpers
// ============================================================

static int rssiToPercent(int8_t rssi) {
    return constrain(map(rssi, -96, -30, 0, 100), 0, 100);
}

static unsigned long percentToBeepInterval(int pct) {
    return map(pct, 0, 100, 1200, 120);   // näher = schneller
}

// Farbverlauf rot → gelb → grün, stufenlos statt in 3 harten Blöcken
static uint16_t signalColorGradient(int pct) {
    uint8_t r, g;
    if (pct < 50) {
        // rot (pct=0) → gelb (pct=50)
        r = 255;
        g = map(pct, 0, 50, 0, 255);
    } else {
        // gelb (pct=50) → grün (pct=100)
        r = map(pct, 50, 100, 255, 0);
        g = 255;
    }
    uint8_t r5 = r >> 3;
    uint8_t g6 = g >> 2;
    return (r5 << 11) | (g6 << 5);
}

// ============================================================
//  Drawing
// ============================================================

static void drawBarFrame() {
    M5.Lcd.drawRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, 6, COL_BAR_BORDER);
}

static void drawStatic() {
    M5.Lcd.fillScreen(COL_SCREEN_BG);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x07E0, COL_SCREEN_BG);
    M5.Lcd.setCursor(8, 10);
    M5.Lcd.print(targetName_);

    M5.Lcd.setTextColor(COL_TEXT_DIM, COL_SCREEN_BG);
    M5.Lcd.setCursor(8, 30);
    M5.Lcd.print(targetMac_);

    // Skalen-Beschriftung unter dem Balken
    M5.Lcd.setTextColor(COL_TEXT_DIM, COL_SCREEN_BG);
    M5.Lcd.setCursor(BAR_X, BAR_Y + BAR_H + 4);
    M5.Lcd.print("FAR");
    M5.Lcd.setCursor(BAR_X + BAR_W - 30, BAR_Y + BAR_H + 4);
    M5.Lcd.print("NEAR");

    M5.Lcd.fillRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, 6, COL_BAR_BG);
    drawBarFrame();

    //M5.Lcd.setTextColor(COL_TEXT_DIM, COL_SCREEN_BG);
    //M5.Lcd.setCursor(4, 125);
    
    // Status bar at bottom
    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
#if HAS_KEYBOARD
    M5.Lcd.print(" esc back to list");
#else
    M5.Lcd.print("hold big:back to list");
#endif
}

static void drawDynamic(int8_t rssi, bool found) {
    // Balken-Innenfläche zurücksetzen (Rahmen bleibt stehen)
    M5.Lcd.fillRoundRect(BAR_X + 1, BAR_Y + 1, BAR_W - 2, BAR_H - 2, 5, COL_BAR_BG);

    M5.Lcd.fillRect(0, 108, 240, 14, COL_SCREEN_BG);

    if (!found) {
        M5.Lcd.setTextColor(0x8C71, COL_SCREEN_BG);
        M5.Lcd.setCursor(4, 105);
        M5.Lcd.print("Signal lost...");
        return;
    }

    int pct = rssiToPercent(rssi);
    uint16_t col = signalColorGradient(pct);

    // Balken füllen, proportional zu pct, von links nach rechts
    int fillW = (BAR_W - 4) * pct / 100;
    if (fillW > 0) {
        M5.Lcd.fillRoundRect(BAR_X + 2, BAR_Y + 2, fillW, BAR_H - 4, 4, col);
    }

    // Zielmarkierung ganz rechts am Ende (100%) — pulsiert leicht, sobald erreicht
    if (pct >= 95) {
        M5.Lcd.drawRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, 6, COL_GREEN);
        M5.Lcd.drawRoundRect(BAR_X - 1, BAR_Y - 1, BAR_W + 2, BAR_H + 2, 7, COL_GREEN);
    } else {
        drawBarFrame();
    }

    // Prozent + dBm-Anzeige
    M5.Lcd.setTextColor(col, COL_SCREEN_BG);
    M5.Lcd.setCursor(4, 105);
    M5.Lcd.printf("%3d%%  %d dBm", pct, rssi);
}

// ============================================================
//  Audio
// ============================================================

static void beepIfNeeded(int8_t rssi, bool found) {
    auto* ms = MenuController::getState();
    if (!ms || !ms->audioEnabled) return;
    if (!found) return;

    unsigned long now = millis();
    int pct = rssiToPercent(rssi);
    unsigned long interval = percentToBeepInterval(pct);

    if (now - lastBeepTime_ < interval) return;
    lastBeepTime_ = now;

    int freq = map(pct, 0, 100, 900, 1600);
    M5.Speaker.setVolume(MenuController::getAlarmVolume());
    M5.Speaker.tone(freq, 40);
}

// ============================================================
//  Public API
// ============================================================

static ReturnTarget returnTarget_ = ReturnTarget::FinderList;

void open(const char* mac, const char* name, ReturnTarget returnTo) {
    if (ScanContext::bleScanEnabled.load()) {
        LOG(LOG_CONTROL, "Approach View — stopping main scan");
        stopBleScan();
    }

    open_          = true;
    staticDrawn_   = false;
    scanStopped_   = false;
    targetMac_     = mac;
    targetName_    = name;
    returnTarget_  = returnTo;   // ← NEU
    smoothedRssi_  = -100;
    lastDrawnRssi_ = -127;
    lastFound_     = false;
    lastScanTime_  = 0;
    lastBeepTime_  = 0;
}

void close() {
    open_ = false;
    if (returnTarget_ == ReturnTarget::SusList) {
        SusDeviceView::open();
    } else {
        FinderListView::open();
    }
}

bool isOpen() { return open_; }

void update() {
    if (!open_) return;

    if (!staticDrawn_) {
        drawStatic();
        staticDrawn_ = true;
    }

    if (!scanStopped_) {
        if (ScanContext::bleScanEnabled.load()) {
            LOG(LOG_CONTROL, "Approach View — stopping main scan");
            stopBleScan();
        }

        LOG(LOG_CONTROL, "Approach View — waiting, scanIsRunning=" + String(ScanContext::scanIsRunning.load()));

        unsigned long waitStart = millis();
        int dots = 0;
        while (ScanContext::scanIsRunning.load() && (millis() - waitStart < 8000)) {
            M5.Lcd.fillRect(0, 108, 240, 14, COL_SCREEN_BG);
            M5.Lcd.setTextColor(0xFFE0, COL_SCREEN_BG);
            M5.Lcd.setCursor(4, 110);
            M5.Lcd.print("Pausing main scan");
            for (int i = 0; i < (dots % 4); i++) M5.Lcd.print(".");
            dots++;
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        scanStopped_  = true;
        lastScanTime_ = millis();
        return;
    }

    beepIfNeeded(smoothedRssi_, lastFound_);

    unsigned long now = millis();
    if (now - lastScanTime_ < 600) return;
    lastScanTime_ = now;

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->clearResults();
    pScan->setActiveScan(false);
    pScan->setPhy(NimBLEScan::Phy::SCAN_ALL);
    pScan->setInterval(BLE_SCAN_INTERVAL);
    pScan->setWindow(BLE_SCAN_WINDOW);
    pScan->setInterval(40);
    pScan->setWindow(40);
    NimBLEScanResults results = pScan->getResults(500);

    bool   found = false;
    int8_t rssi  = -100;

    for (int i = 0; i < results.getCount(); i++) {
        const NimBLEAdvertisedDevice* d = results.getDevice(i);
        if (d && d->getAddress().toString() == targetMac_.c_str()) {
            rssi  = d->getRSSI();
            found = true;
            break;
        }
    }

    if (found) {
        smoothedRssi_ = (smoothedRssi_ + rssi) / 2;
    }
    lastFound_ = found;

    drawDynamic(smoothedRssi_, found);
    lastDrawnRssi_ = smoothedRssi_;
}

} // namespace ApproachView