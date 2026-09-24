#include "gatt_console_view.h"
#include <M5Unified.h>
#include "infrastructure/ble/gatt_console.h"
#include "ui/conview/connected_device_view.h"


namespace GattConsoleView {

enum class State { CLOSED, CONNECTING, CHAR_LIST, HEXINPUT };

static constexpr int MENU_W = 240;
static constexpr int MENU_H = 135;
static constexpr int ROW_H  = 11;

static constexpr uint16_t COL_CURSOR    = 0x07E0;
static constexpr uint16_t COL_STATUSBAR = 0x5ACB;

static State state_ = State::CLOSED;
static State lastDrawnState_ = State::CLOSED;
static int   cursorIdx_ = 0;
static std::vector<GattConsole::CharInfo> chars_;
static String hexBuffer_;
static String lastResponse_;
static std::string sessionMac_;
static uint8_t sessionAddrType_ = 0;

static void connectTask(void* param) {
    bool ok = GattConsole::connect(sessionMac_, sessionAddrType_);
    if (ok) {
        chars_ = GattConsole::listWritableChars();
        cursorIdx_ = 0;
        state_ = State::CHAR_LIST;
    } else {
        state_ = State::CLOSED;   // Connect fehlgeschlagen -> zurück zur Liste
    }
    vTaskDelete(nullptr);
}

void tick() {
    if (state_ == State::CLOSED) return;
    if (state_ == lastDrawnState_) return;   // nur bei echtem Zustandswechsel neu zeichnen

    lastDrawnState_ = state_;
    draw();
}

void open(const std::string& mac, uint8_t addrType) {
    sessionMac_      = mac;
    sessionAddrType_ = addrType;
    state_ = State::CONNECTING;
    hexBuffer_.clear();
    lastResponse_.clear();

    xTaskCreatePinnedToCore(connectTask, "GattConsoleConn", 8192, nullptr, 4, nullptr, 1);
    draw();
}

bool isOpen()      { return state_ != State::CLOSED; }
bool isInputOpen() { return state_ == State::HEXINPUT; }

void navigateNext() {
    if (state_ != State::CHAR_LIST || chars_.empty()) return;
    cursorIdx_ = (cursorIdx_ + 1) % chars_.size();
    draw();
}

void navigatePrev() {
    if (state_ != State::CHAR_LIST || chars_.empty()) return;
    cursorIdx_ = (cursorIdx_ - 1 + chars_.size()) % chars_.size();
    draw();
}

void selectCurrent() {
    if (state_ == State::CHAR_LIST && !chars_.empty()) {
        state_ = State::HEXINPUT;
        lastResponse_.clear();
        draw();
        return;
    }

    if (state_ == State::HEXINPUT) {
        bool ok = GattConsole::sendHex(chars_[cursorIdx_].uuid, hexBuffer_.c_str());
        if (ok) {
            vTaskDelay(pdMS_TO_TICKS(150));  // kurz auf Notify-Antwort warten
            String resp = GattConsole::getLastResponseHex().c_str();
            lastResponse_ = resp.isEmpty() ? "(keine Antwort)" : resp;
        } else {
            lastResponse_ = "SEND FAILED";
        }
        draw();
    }
}

void appendChar(char c) {
    if (state_ != State::HEXINPUT) return;
    bool isHex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                 (c >= 'A' && c <= 'F') || c == ' ';
    if (!isHex) return;
    if (hexBuffer_.length() >= 60) return;  // Sicherheitslimit
    hexBuffer_ += c;
    draw();
}

void backspace() {
    if (state_ != State::HEXINPUT || hexBuffer_.isEmpty()) return;
    hexBuffer_.remove(hexBuffer_.length() - 1);
    draw();
}

void close() {
    if (state_ == State::HEXINPUT) {
        state_ = State::CHAR_LIST;   // ein Schritt zurück
        return;
    }
    if (state_ == State::CHAR_LIST || state_ == State::CONNECTING) {
        GattConsole::disconnectSession();
        state_ = State::CLOSED;
    }
}

static void drawConnecting() {
    M5.Lcd.fillScreen(0x0020);
    M5.Lcd.setTextColor(COL_CURSOR, 0x0020);
    M5.Lcd.setCursor(4, 4);
    M5.Lcd.print("GATT CONSOLE");
    M5.Lcd.setCursor(4, 40);
    M5.Lcd.print("Connecting...");
}

static void drawCharList() {
    M5.Lcd.fillScreen(0x0020);
    M5.Lcd.setTextColor(COL_CURSOR, 0x0020);
    M5.Lcd.setCursor(4, 2);
    M5.Lcd.printf("WRITABLE CHARS (%d)", (int)chars_.size());

    int y = 16;
    for (size_t i = 0; i < chars_.size() && i < 8; i++) {
        bool selected = ((int)i == cursorIdx_);
        uint16_t bg = selected ? 0x0341 : 0x0020;
        M5.Lcd.fillRect(0, y, MENU_W, ROW_H, bg);
        M5.Lcd.setTextColor(0x07E0, bg);
        M5.Lcd.setCursor(4, y + 1);
        M5.Lcd.print(chars_[i].uuid.c_str());   // volle UUID, keine Kürzung
        y += ROW_H;
    }

    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
    M5.Lcd.print("^:up v:down ok:select esc:back");
}

static void drawInput() {
    M5.Lcd.fillScreen(0x0020);
    M5.Lcd.setTextColor(COL_CURSOR, 0x0020);
    M5.Lcd.setCursor(4, 4);
    String uuidShort = chars_[cursorIdx_].uuid.c_str();
    if (uuidShort.length() > 34) uuidShort = uuidShort.substring(0, 34) + "...";
    M5.Lcd.print(uuidShort);

    M5.Lcd.setTextColor(0x8C71, 0x0020);
    M5.Lcd.setCursor(4, 30);
    M5.Lcd.print("> " + hexBuffer_ + "_");

    M5.Lcd.setCursor(4, 46);
    M5.Lcd.printf("Bytes: ~%d", (hexBuffer_.length() + 1) / 3);

    if (!lastResponse_.isEmpty()) {
        M5.Lcd.setTextColor(0x07E0, 0x0020);
        M5.Lcd.setCursor(4, 62);
        M5.Lcd.print("Response:");
        M5.Lcd.setCursor(4, 72);
        M5.Lcd.print(lastResponse_);
    }

    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
    M5.Lcd.print("ok:send esc:back del:backspace");
}

void draw() {
    switch (state_) {
        case State::CONNECTING: drawConnecting(); break;
        case State::CHAR_LIST:  drawCharList();   break;
        case State::HEXINPUT:      drawInput();      break;
        default: break;
    }
}

} // namespace GattConsoleView
