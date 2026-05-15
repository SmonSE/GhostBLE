#include "menu_controller.h"

#include <M5Unified.h>
#include "infrastructure/logging/logger.h"
#include "app/context/device_context.h"
#include "app/context/scan_context.h"
#include "app/context/network_context.h"
#include "app/context/ui_context.h"
#include "app/context/globals.h"

#include "config/ui_config.h"

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
static constexpr uint16_t COL_STATUSBAR = 0x0010;   // very dark for status bar
static constexpr uint16_t COL_HINT      = 0x2945;   // dim hint text


// ── Menu item definition ──────────────────────────────────────
struct MenuItem {
    MenuItemType  type;
    const char*   label;
    bool*         value;       // pointer into MenuState (or nullptr for actions/sections)
    bool          subItem;     // true = indented (audio sub-category)
    bool*         parentOn;    // if non-null, dim when parent is off
    void          (*action)(); // for Action type
    const char*   valueHint;   // for ValueInfo / Action (shown right-aligned)
};

// ── State ─────────────────────────────────────────────────────
static MenuState*  state_      = nullptr;
static bool        menuOpen_   = false;
static int         cursorIdx_  = 0;      // absolute item index
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

// Build item table — called in init()
static void buildItems() {
    itemCount_ = 0;

    auto& s = *state_;

    auto section = [&](const char* label) {
        items_[itemCount_++] = { MenuItemType::Section, label, nullptr, false, nullptr, nullptr, nullptr };
    };
    auto toggle = [&](const char* label, bool& val, bool sub = false, bool* parent = nullptr) {
        items_[itemCount_++] = { MenuItemType::Toggle, label, &val, sub, parent, nullptr, nullptr };
    };
    auto action = [&](const char* label, void(*fn)(), const char* hint = nullptr) {
        items_[itemCount_++] = { MenuItemType::Action, label, nullptr, false, nullptr, fn, hint };
    };
    auto info = [&](const char* label, const char* hint) {
        items_[itemCount_++] = { MenuItemType::ValueInfo, label, nullptr, false, nullptr, nullptr, hint };
    };
    auto spacer = [&]() { 
        items_[itemCount_++] = { MenuItemType::Spacer, "", nullptr, false, nullptr, nullptr, nullptr };
};

    // ── AUDIO ALERTS ─────────────────────────────────────────
    section("AUDIO ALERTS");
    toggle("Audio",           s.audioEnabled);
    toggle("Suspicious",      s.audioSuspicious,     true, &s.audioEnabled);
    toggle("Flock camera",    s.audioFlock,          true, &s.audioEnabled);
    toggle("Drone",           s.audioDrone,          true, &s.audioEnabled);
    toggle("Flipper Zero",    s.audioFlipper,        true, &s.audioEnabled);
    toggle("PwnBeacon",       s.audioPwnBeacon,      true, &s.audioEnabled);

    // ── FEATURES ─────────────────────────────────────────────
    //section("FEATURES");
    //toggle("Research mode",   s.researchModeEnabled);
    //action("Screenshot",      nullptr, "ENTER");

    // Spacer fo last row to avoid drawing cursor on top of status bar
    //spacer();
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

static void drawRow(int rowY, const MenuItem& item, bool selected) {
    bool dimmed = item.parentOn && !(*item.parentOn);

    // Background
    uint16_t bg = selected ? COL_SELECTED : COL_BG;
    M5.Lcd.fillRect(MENU_X, rowY, MENU_W, ROW_H, bg);

    if (item.type == MenuItemType::Section) {
        // Section header — no cursor, muted color, small text
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(COL_SECTION, bg);
        M5.Lcd.setCursor(MENU_X + 2, rowY + 2);
        M5.Lcd.print(item.label);
        return;
    }

    int xOff = item.subItem ? MENU_X + INDENT_W + 8 : MENU_X + 8;

    // Cursor
    M5.Lcd.setTextSize(1);
    if (selected) {
        M5.Lcd.setTextColor(COL_CURSOR, bg);
        M5.Lcd.setCursor(MENU_X + 1, rowY + 2);
        M5.Lcd.print(">");
    }

    // Label
    uint16_t labelCol = dimmed ? COL_LABEL_DIM
                      : selected ? COL_LABEL_SEL
                      : COL_LABEL;
    M5.Lcd.setTextColor(labelCol, bg);
    M5.Lcd.setCursor(xOff, rowY + 2);
    M5.Lcd.print(item.label);

    // Right side: checkbox or hint
    int rightX = MENU_W - 14;
    if (item.type == MenuItemType::Toggle && item.value) {
        drawCheckbox(rightX, rowY + 2, *item.value);
    } else if (item.valueHint) {
        M5.Lcd.setTextColor(COL_VALUE, bg);
        // right-align hint
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

void navigateUp() {
    if (!menuOpen_) return;
    int prev = cursorIdx_;
    do {
        cursorIdx_--;
        if (cursorIdx_ < 0) cursorIdx_ = itemCount_ - 1;
    } while (items_[cursorIdx_].type == MenuItemType::Section &&
             cursorIdx_ != prev);

    // Scroll up if needed
    if (cursorIdx_ < scrollOff_) {
        scrollOff_ = cursorIdx_;
    }
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

    // Scroll down if needed
    if (cursorIdx_ >= scrollOff_ + ROWS_VISIBLE) {
        scrollOff_ = cursorIdx_ - ROWS_VISIBLE + 1;
    }
    draw();
}

void selectCurrent() {
    if (!menuOpen_ || cursorIdx_ < 0 || cursorIdx_ >= itemCount_) return;

    MenuItem& item = items_[cursorIdx_];

    if (item.type == MenuItemType::Toggle && item.value) {
        *item.value = !(*item.value);
        beep(*item.value ? 1760 : 880, 60);

        // ── Side effects ──────────────────────────────────────
        /*
        if (item.value == &state_->bleScanEnabled) {
            onLongPress();
        } else if (item.value == &state_->wardrivingEnabled) {
            toggleWardriving();
        } else if (item.value == &state_->wifiEnabled) {
            toggleWiFi();
        } else if (item.value == &state_->researchMode) {
            UIContext::isResearchModeActive.store(state_->researchMode);
            LOG(LOG_CONTROL, state_->researchMode
                ? "Research Mode ON" : "Research Mode OFF");
        }
        */

        LOG(LOG_CONTROL, String("[Menu] ") + item.label +
            ((*item.value) ? " ON" : " OFF"));

    } else if (item.type == MenuItemType::Action && item.action) {
        beep(1046, 60);
        item.action();
    }

    draw();
}

void draw() {
    if (!menuOpen_) return;

    M5.Lcd.setTextSize(1);

    // Title bar
    M5.Lcd.fillRect(0, 0, MENU_W, ROW_H + 1, 0x000F);
    M5.Lcd.setTextColor(COL_CURSOR, 0x000F);
    M5.Lcd.setCursor(2, 2);
    M5.Lcd.print("GhostBLE Menu");

    // LV indicator right side of title
    String lv = "LV" + String(DeviceContext::xpManager.getLevel());
    M5.Lcd.setTextColor(COL_VALUE, 0x000F);
    M5.Lcd.setCursor(MENU_W - lv.length() * 6 - 2, 2);
    M5.Lcd.print(lv);

    // Draw visible rows
    int drawY = ROW_H + 2;
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
    M5.Lcd.setTextColor(COL_HINT, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);

#if HAS_KEYBOARD
    M5.Lcd.print("arr navigate  ENTER toggle  FN close");
#else
    M5.Lcd.print("A:down  B:toggle  M5(long):close");
#endif

    // Scroll indicators
    if (scrollOff_ > 0) {
        M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
        M5.Lcd.setCursor(MENU_W - 14, MENU_H - ROW_H + 2);
        M5.Lcd.print("^");
    }
    if (scrollOff_ + ROWS_VISIBLE < itemCount_) {
        M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
        M5.Lcd.setCursor(MENU_W - 8, MENU_H - ROW_H + 2);
        M5.Lcd.print("v");
    }
}

MenuState* getState() { return state_; }

} // namespace MenuController
