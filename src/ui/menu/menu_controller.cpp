#include "menu_controller.h"

#include <M5Unified.h>
#include "infrastructure/logging/logger.h"
#include "infrastructure/platform/hardware_config.h"
#include "app/context/device_context.h"
#include "app/context/scan_context.h"
#include "app/context/network_context.h"
#include "app/context/ui_context.h"
#include "app/context/globals.h"

#include "config/ui_config.h"
#include "config/scan_config.h"

#include "ui/overlay/draw_overlay.h"
#include "ui/expression/show_expression.h"
#include "ui/menu/menu_controller.h"
#include "ui/icons/scan_icon.h"

#include "assets/nibblesFront.h"
#include "assets/nibblesHappy.h"


// Forward declarations for actions defined in GhostBLE.ino
extern void toggleWiFi();
extern void toggleWardriving();
extern void switchGPSSource();
extern void onLongPress();

namespace MenuController {

// ── Layout constants ──────────────────────────────────────────
static constexpr int MENU_X        = 0;
static constexpr int MENU_Y        = 0;
static constexpr int MENU_W        = 240;
static constexpr int MENU_H        = 135;
static constexpr int ROW_H         = 11;    // pixels per row
static constexpr int ROWS_VISIBLE  = 11;    // rows on screen at once
static constexpr int INDENT_W      = 8;     // sub-item indent pixels

// ── Colors (RGB565) ───────────────────────────────────────────
static constexpr uint16_t COL_BG        = 0x0020;   // very dark blue
static constexpr uint16_t COL_SELECTED  = 0x0341;   // dark teal highlight
static constexpr uint16_t COL_CURSOR    = 0x07E0;   // green
static constexpr uint16_t COL_LABEL     = 0x8C71;   // light gray
static constexpr uint16_t COL_LABEL_SEL = 0x07E0;   // green when selected
static constexpr uint16_t COL_LABEL_DIM = 0x39C7;   // dimmed (master off)
static constexpr uint16_t COL_SECTION   = 0x2945;   // muted blue-gray
static constexpr uint16_t COL_ON        = 0x07E0;   // green — filled checkbox
static constexpr uint16_t COL_OFF       = 0x39C7;   // gray  — empty checkbox
static constexpr uint16_t COL_VALUE     = 0x2945;   // dim blue for value text
static constexpr uint16_t COL_STATUSBAR = 0x5ACB;   // very dark for status bar
static constexpr uint16_t COL_HINT      = 0x2945;   // dim hint text


// ── Menu item definition ──────────────────────────────────────
struct MenuItem {
    MenuItemType  type;
    const char*   label;
    bool*         value;
    bool          (*getState)();
    bool          subItem;
    bool*         parentOn;
    void          (*action)();
    const char*   valueHint;

    // ── Slider-spezifisch ──────────────────────────────────
    uint8_t*      sliderValue = nullptr;   // Pointer auf den Wert (z.B. Helligkeit)
    uint8_t       sliderMin   = 0;
    uint8_t       sliderMax   = 255;
    uint8_t       sliderStep  = 17;        // ~15 Schritte über 0-255
    void          (*onSliderChange)(uint8_t) = nullptr;  // z.B. M5.Lcd.setBrightness
};

// ── State ─────────────────────────────────────────────────────
static MenuState*  state_      = nullptr;
static bool        menuOpen_   = false;
static int         cursorIdx_  = 1;      // absolute item index
static int         scrollOff_  = 0;      // first visible item index

// ── Item table ────────────────────────────────────────────────
// Built after init() because items reference state_ fields.
static MenuItem items_[40];
static int      itemCount_ = 0;

// Helper to play a tone if audio is enabled
static void beep(int freq, int dur) {
#if HAS_SPEAKER
    if (state_ && state_->audioEnabled) {
        M5.Speaker.setVolume(30);
        M5.Speaker.tone(freq, dur);
    }
#endif
}

// ── Brightness ───────────────────────────────────────────────
static uint8_t brightness_ = 128;

static void applyBrightness(uint8_t val) {
    M5.Lcd.setBrightness(val);
}

// Build item table — called in init()
static void buildItems() {
    itemCount_ = 0;
    auto& s = *state_;

    auto section = [&](const char* label) {
        items_[itemCount_++] = { MenuItemType::Section, label, nullptr, nullptr, false, nullptr, nullptr, nullptr };
    };
    // einfacher Toggle (bool* aus MenuState — unverändert wie bisher)
    auto toggle = [&](const char* label, bool& val, bool sub = false, bool* parent = nullptr) {
        items_[itemCount_++] = { MenuItemType::Toggle, label, &val, nullptr, sub, parent, nullptr, nullptr };
    };
    // Toggle mit atomarem Getter + Action-Funktion (Side-Effect inklusive)
    auto toggleAction = [&](const char* label, bool (*getter)(), void (*fn)(), bool sub = false, bool* parent = nullptr) {
        items_[itemCount_++] = { MenuItemType::Toggle, label, nullptr, getter, sub, parent, fn, nullptr };
    };
    auto action = [&](const char* label, void(*fn)(), const char* hint = nullptr) {
        items_[itemCount_++] = { MenuItemType::Action, label, nullptr, nullptr, false, nullptr, fn, hint };
    };
    auto spacer = [&]() {
        items_[itemCount_++] = { MenuItemType::Spacer, "", nullptr, nullptr, false, nullptr, nullptr, nullptr };
    };

    auto info = [&](const char* label, const char* hint) {
    items_[itemCount_++] = { MenuItemType::ValueInfo, label, nullptr, nullptr, false, nullptr, nullptr, hint };
    };

    auto slider = [&](const char* label, uint8_t& val, uint8_t minV, uint8_t maxV,
                   uint8_t step, void (*onChange)(uint8_t) = nullptr) {
        MenuItem item{};
        item.type            = MenuItemType::Slider;
        item.label           = label;
        item.sliderValue     = &val;
        item.sliderMin       = minV;
        item.sliderMax       = maxV;
        item.sliderStep      = step;
        item.onSliderChange  = onChange;
        items_[itemCount_++] = item;
    };

    // ── SCAN ─────────────────────────────────────────────────
    section("SCAN");
    //toggleAction("BLE Scan", []() { return ScanContext::bleScanEnabled.load(); }, toggleScanMode);
    toggleAction("Research Mode", []() { return UIContext::isResearchModeActive.load(); }, []() {
        bool newVal = !UIContext::isResearchModeActive.load();
        UIContext::isResearchModeActive.store(newVal);
        showResearchMode();
    });

    // ── WIRELESS ─────────────────────────────────────────────
    section("WIRELESS");
    toggleAction("WiFi Web UI", []() { return NetworkContext::wifiStarted; }, toggleWiFi);

    // ── WARDRIVE ─────────────────────────────────────────────
    section("WARDRIVE");
    toggleAction("Wardriving", []() { return NetworkContext::wardrivingEnabled.load(); }, toggleWardriving);

    // "GPS Source" als reine Info-Zeile / Sub-Section-Label
    info("GPS Source", "");

    toggleAction("Grove", []() { return NetworkContext::isGPSSourceGrove(); }, NetworkContext::setGPSSourceGrove, true);
    #if defined(LORA_CS_PIN)
    toggleAction("LoRa Cap", []() { return NetworkContext::isGPSSourceLora(); }, NetworkContext::setGPSSourceLora, true);
    #endif

    // ── AUDIO ALERTS ─────────────────────────────────────────
    section("AUDIO ALERTS");
    toggle("Audio",           s.audioEnabled);
    toggle("Suspicious",      s.audioSuspicious, true, &s.audioEnabled);
    toggle("Flock camera",    s.audioFlock,      true, &s.audioEnabled);
    toggle("Drone",           s.audioDrone,      true, &s.audioEnabled);
    toggle("Flipper Zero",    s.audioFlipper,    true, &s.audioEnabled);
    toggle("PwnBeacon",       s.audioPwnBeacon,  true, &s.audioEnabled);


    // ── DISPLAY ──────────────────────────────────────────────
    section("DISPLAY");
    slider("Brightness", brightness_, 10, 255, 17, applyBrightness);
}

// ── Drawing helpers ───────────────────────────────────────────

static void drawCheckbox(int x, int y, bool on) {
    int sz = 7;
    if (on) {
        M5.Lcd.fillRect(x, y, sz, sz, COL_ON);
    } else {
        M5.Lcd.drawRect(x, y, sz, sz, COL_OFF);
    }
}

static void drawSlider(int rowY, const MenuItem& item, bool selected) {
    int barW = 60;
    int barH = 3;
    int barX = MENU_W - barW - 40;   // rechts ausgerichtet, mit Platz für Wert-Text daneben
    int barY = rowY + 4;

    // Balken-Hintergrund
    M5.Lcd.drawRect(barX, barY, barW, barH + 2, COL_OFF);

    // Gefüllter Anteil
    uint8_t val = *item.sliderValue;
    int fillW = ((val - item.sliderMin) * (barW - 2)) /
                (item.sliderMax - item.sliderMin);
    if (fillW > 0) {
        M5.Lcd.fillRect(barX + 1, barY + 1, fillW, barH, COL_ON);
    }

    // Cursor "|" an der aktuellen Position
    int cursorX = barX + fillW;
    M5.Lcd.setTextColor(selected ? COL_LABEL_SEL : COL_CURSOR, COL_BG);
    M5.Lcd.setCursor(cursorX, rowY + 2);
    M5.Lcd.print("|");

    // Wert als Zahl rechts vom Balken
    M5.Lcd.setTextColor(COL_VALUE, COL_BG);
    M5.Lcd.setCursor(barX + barW + 8, rowY + 2);
    M5.Lcd.print(String(val));
}

static void drawRow(int rowY, const MenuItem& item, bool selected) {
    bool dimmed = item.parentOn && !(*item.parentOn);

    uint16_t bg = selected ? COL_SELECTED : COL_BG;
    M5.Lcd.fillRect(MENU_X, rowY, MENU_W, ROW_H, bg);

    if (item.type == MenuItemType::Section) {
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(COL_SECTION, bg);
        M5.Lcd.setCursor(MENU_X + 2, rowY + 2);
        M5.Lcd.print(item.label);
        return;
    }

    int xOff = item.subItem ? MENU_X + INDENT_W + 8 : MENU_X + 8;

    M5.Lcd.setTextSize(1);
    if (selected) {
        M5.Lcd.setTextColor(COL_CURSOR, bg);
        M5.Lcd.setCursor(MENU_X + 1, rowY + 2);
        M5.Lcd.print(">");
    }

    uint16_t labelCol = dimmed ? COL_LABEL_DIM
                      : selected ? COL_LABEL_SEL
                      : COL_LABEL;
    M5.Lcd.setTextColor(labelCol, bg);
    M5.Lcd.setCursor(xOff, rowY + 2);
    M5.Lcd.print(item.label);

    int rightX = MENU_W - 14;
    if (item.type == MenuItemType::Toggle) {
        bool isOn = item.value ? *item.value
                  : (item.getState ? item.getState() : false);
        drawCheckbox(rightX, rowY + 2, isOn);
    } else if (item.type == MenuItemType::Slider) {
        drawSlider(rowY, item, selected); 
    } else if (item.valueHint) {
        M5.Lcd.setTextColor(COL_VALUE, bg);
        int hintW = strlen(item.valueHint) * 6;
        M5.Lcd.setCursor(MENU_W - hintW - 2, rowY + 2);
        M5.Lcd.print(item.valueHint);
    }
}

// ── Public API ────────────────────────────────────────────────

void init(MenuState* s) {
    state_ = s;
    buildItems();
}

void open() {
    menuOpen_ = true;
    cursorIdx_ = 0;
    scrollOff_ = 0;
    // Skip past first section header to land on first real item
    while (cursorIdx_ < itemCount_ &&
           items_[cursorIdx_].type == MenuItemType::Section) {
        cursorIdx_++;
    }
    draw();
}

void close() {
    menuOpen_ = false;
    // Redraw normal UI
    M5.Lcd.fillScreen(0x00C4);
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    // All other stuff needs to be set here also XP bar missing
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
    showScanIcon();
    drawXPBar(LEVEL_TEXT_X, BOTTOM_BAR_Y, true);
}

bool isOpen() { return menuOpen_; }

static void ensureCursorVisible() {
    // Scroll up if cursor is above the visible window
    if (cursorIdx_ < scrollOff_) {
        int newOff = cursorIdx_;
        while (newOff > 0 && items_[newOff - 1].type == MenuItemType::Section) {
            newOff--;
        }
        scrollOff_ = newOff;
    }
    // Scroll down if cursor is below the visible window
    else if (cursorIdx_ >= scrollOff_ + ROWS_VISIBLE) {
        scrollOff_ = cursorIdx_ - ROWS_VISIBLE + 1;
    }
}

void navigateUp() {
    if (!menuOpen_) return;
    int prev = cursorIdx_;
    do {
        cursorIdx_--;
        if (cursorIdx_ < 0) cursorIdx_ = itemCount_ - 1;
    } while (items_[cursorIdx_].type == MenuItemType::Section &&
             cursorIdx_ != prev);

    ensureCursorVisible();
    draw();
}

void navigateDown() {
    if (!menuOpen_) return;
    int prev = cursorIdx_;
    do {
        cursorIdx_++;
        if (cursorIdx_ >= itemCount_) cursorIdx_ = 0;
    } while (items_[cursorIdx_].type == MenuItemType::Section &&
             cursorIdx_ != prev);

    ensureCursorVisible();
    draw();
}

void adjustLeft() {
    if (!menuOpen_ || cursorIdx_ < 0 || cursorIdx_ >= itemCount_) return;
    MenuItem& item = items_[cursorIdx_];
    if (item.type != MenuItemType::Slider || !item.sliderValue) return;

    int newVal = (int)*item.sliderValue - item.sliderStep;
    if (newVal < item.sliderMin) newVal = item.sliderMin;
    *item.sliderValue = (uint8_t)newVal;

    if (item.onSliderChange) item.onSliderChange(*item.sliderValue);
    beep(880, 30);
    draw();
}

void adjustRight() {
    if (!menuOpen_ || cursorIdx_ < 0 || cursorIdx_ >= itemCount_) return;
    MenuItem& item = items_[cursorIdx_];
    if (item.type != MenuItemType::Slider || !item.sliderValue) return;

    int newVal = (int)*item.sliderValue + item.sliderStep;
    if (newVal > item.sliderMax) newVal = item.sliderMax;
    *item.sliderValue = (uint8_t)newVal;

    if (item.onSliderChange) item.onSliderChange(*item.sliderValue);
    beep(1760, 30);
    draw();
}

void selectCurrent() {
    if (!menuOpen_ || cursorIdx_ < 0 || cursorIdx_ >= itemCount_) return;

    MenuItem& item = items_[cursorIdx_];

    if (item.type == MenuItemType::Toggle) {
        if (item.value) {
            // einfacher bool* Toggle (Audio-Flags)
            *item.value = !(*item.value);
            beep(*item.value ? 1760 : 880, 60);
            LOG(LOG_CONTROL, String("[Menu] ") + item.label + ((*item.value) ? " ON" : " OFF"));
        } else if (item.action) {
            // atomic-backed Toggle mit Side-Effect
            item.action();
            bool nowOn = item.getState ? item.getState() : false;
            beep(nowOn ? 1760 : 880, 60);
            LOG(LOG_CONTROL, String("[Menu] ") + item.label + (nowOn ? " ON" : " OFF"));
        }
    } else if (item.type == MenuItemType::Action && item.action) {
        beep(1046, 60);
        item.action();
    }

    draw();
}

void draw() {
    if (!menuOpen_) return;
    
    M5.Lcd.setTextSize(1);

    // Draw visible rows — starts at top since no title bar
    int drawY = MENU_Y;   // = 0
    for (int i = scrollOff_; i < itemCount_ && i < scrollOff_ + ROWS_VISIBLE; i++) {
        drawRow(drawY, items_[i], (i == cursorIdx_));
        drawY += ROW_H;
    }

    // Fill remaining space
    if (drawY < MENU_H - ROW_H) {
        M5.Lcd.fillRect(0, drawY, MENU_W, MENU_H - ROW_H - drawY, COL_BG);
    }

    // Status bar at bottom
    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);

#if HAS_KEYBOARD
    M5.Lcd.print(" ^:up  v:down  ok:enable/disable");
#else
    M5.Lcd.print("A:down  B:toggle  M5(long):close");
#endif
}

MenuState* getState() { return state_; }

} // namespace MenuController
