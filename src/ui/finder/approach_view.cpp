// approach_view.cpp
#include "approach_view.h"
#include <NimBLEDevice.h>
#include <M5Unified.h>

#include "ui/menu/menu_controller.h"
#include "ui/finder/finder_list_view.h"


namespace ApproachView {

// ── State ────────────────────────────────────────────────────
static bool    open_          = false;
static bool    staticDrawn_   = false;
static String  targetMac_;
static String  targetName_;
static int8_t  smoothedRssi_  = -100;
static int8_t  lastDrawnRssi_ = -127;
static bool    lastFound_     = false;
static unsigned long lastScanTime_ = 0;
static unsigned long lastBeepTime_ = 0;

// ── Color (RGB565) ─────────────────────────────────────────
static constexpr uint16_t COL_BG_DARK   = 0x0120;
static constexpr uint16_t COL_GRID      = 0x0360;
static constexpr uint16_t COL_BORDER    = 0x07E0;
static constexpr uint16_t COL_SCREEN_BG = 0x0020;
static constexpr uint16_t COL_TEXT_DIM  = 0x2945;

// ── Geometry ────────────────────────────────────────
static constexpr int CAP_X = 20;
static constexpr int CAP_Y = 18;
static constexpr int CAP_W = 200;
static constexpr int CAP_H = 88;
static constexpr int CAP_R = 40;
static constexpr int GRID_STEP = 12;

// ============================================================
//  Helpers
// ============================================================

static int rssiToPercent(int8_t rssi) {

    //return constrain(map(rssi, -88, -40, 0, 100), 0, 100);
    return constrain(map(rssi, -90, -42, 0, 100), 0, 100);
}

static unsigned long percentToBeepInterval(int pct) {
    return map(pct, 0, 100, 1200, 120);   // näher = schneller
}

static uint16_t signalColor(int pct) {
    if (pct > 66) return 0x07E0;   // green
    if (pct > 33) return 0xFFE0;   // yellow
    return 0xF800;                 // red
}

static void drawGrid() {
    M5.Lcd.setClipRect(CAP_X, CAP_Y, CAP_W, CAP_H);
    for (int x = CAP_X; x < CAP_X + CAP_W; x += GRID_STEP) {
        M5.Lcd.drawFastVLine(x, CAP_Y, CAP_H, COL_GRID);
    }
    for (int y = CAP_Y; y < CAP_Y + CAP_H; y += GRID_STEP) {
        M5.Lcd.drawFastHLine(CAP_X, y, CAP_W, COL_GRID);
    }
    M5.Lcd.clearClipRect();
}

// ── Crosshair-Size ────────────────────────────────────────
static constexpr int CROSSHAIR_SIZE = 8;

static void drawCapsuleFrame() {
    M5.Lcd.fillRoundRect(CAP_X, CAP_Y, CAP_W, CAP_H, CAP_R, COL_BG_DARK);
    drawGrid();
    M5.Lcd.drawRoundRect(CAP_X, CAP_Y, CAP_W, CAP_H, CAP_R, COL_BORDER);

    int ccx = CAP_X + CAP_W / 2;
    int ccy = CAP_Y + CAP_H / 2;
    M5.Lcd.drawLine(ccx - CROSSHAIR_SIZE, ccy, ccx + CROSSHAIR_SIZE, ccy, 0xF800);
    M5.Lcd.drawLine(ccx, ccy - CROSSHAIR_SIZE, ccx, ccy + CROSSHAIR_SIZE, 0xF800);
}

// ============================================================
//  Drawing
// ============================================================

static void drawStatic() {
    M5.Lcd.fillScreen(COL_SCREEN_BG);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x07E0, COL_SCREEN_BG);
    M5.Lcd.setCursor(4, 2);
    M5.Lcd.print(targetName_);

    M5.Lcd.setTextColor(COL_TEXT_DIM, COL_SCREEN_BG);
    M5.Lcd.setCursor(120, 2);
    M5.Lcd.print(targetMac_);

    drawCapsuleFrame();

    M5.Lcd.setTextColor(COL_TEXT_DIM, COL_SCREEN_BG);
    M5.Lcd.setCursor(4, 125);
    M5.Lcd.print("C / ESC back to list");
}


static void drawDynamic(int8_t rssi, bool found) {
    drawCapsuleFrame();

    M5.Lcd.fillRect(0, 108, 240, 14, COL_SCREEN_BG);

    if (!found) {
        M5.Lcd.setTextColor(0x8C71, COL_SCREEN_BG);
        M5.Lcd.setCursor(4, 110);
        M5.Lcd.print("Signal lost...");
        return;
    }

    int pct = rssiToPercent(rssi);
    uint16_t col = signalColor(pct);

    // Targetpoint: nearer = closer to center, farther = further away
    int ccx = CAP_X + CAP_W / 2;
    int ccy = CAP_Y + CAP_H / 2;
    int maxR = min(CAP_W, CAP_H) / 1 - 8; // 6px to border, 2px to crosshair

    int dist = map(pct, 0, 100, maxR, 4);
    float angle = random(0, 360) * PI / 180.0f;
    int px = ccx + (int)(dist * cos(angle));
    int py = ccy + (int)(dist * sin(angle) * 0.5f);

    M5.Lcd.setClipRect(CAP_X, CAP_Y, CAP_W, CAP_H);
    M5.Lcd.fillCircle(px, py, 4, col);
    M5.Lcd.drawCircle(px, py, 8, col);
    M5.Lcd.clearClipRect();

    M5.Lcd.setTextColor(col, COL_SCREEN_BG);
    M5.Lcd.setCursor(4, 110);
    M5.Lcd.printf("%3d%%  %d dBm", pct, rssi);

    M5.Lcd.setCursor(150, 110);
    if (rssi > lastDrawnRssi_ + 2)       M5.Lcd.print("^ closer");
    else if (rssi < lastDrawnRssi_ - 2)  M5.Lcd.print("v farther");
    else                                 M5.Lcd.print("- steady");
}

// ============================================================
//  Audio
// ============================================================

static void beepIfNeeded(int8_t rssi, bool found) {
    auto* ms = MenuController::getState();
    if (!ms || !ms->audioEnabled) return;   // Mute respektiert
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

void open(const char* mac, const char* name) {
    open_          = true;
    staticDrawn_   = false;
    targetMac_     = mac;
    targetName_    = name;
    smoothedRssi_  = -100;
    lastDrawnRssi_ = -127;
    lastFound_     = false;
    lastScanTime_  = 0;
    lastBeepTime_  = 0;
}

void close() {
    open_ = false;
    FinderListView::open();
}

bool isOpen() { return open_; }

void update() {
    if (!open_) return;

    if (!staticDrawn_) {
        drawStatic();
        staticDrawn_ = true;
    }

    beepIfNeeded(smoothedRssi_, lastFound_);

    unsigned long now = millis();
    if (now - lastScanTime_ < 1200) return;
    lastScanTime_ = now;

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->clearResults();
    pScan->setActiveScan(false);
    NimBLEScanResults results = pScan->getResults(700);

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
        smoothedRssi_ = (smoothedRssi_ * 3 + rssi) / 4;   // Glättung
    }
    lastFound_ = found;

    drawDynamic(smoothedRssi_, found);
    lastDrawnRssi_ = smoothedRssi_;
}

} // namespace ApproachView
