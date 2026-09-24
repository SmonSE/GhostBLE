#pragma once
#include <string>

namespace GattConsoleView {
    void open(const std::string& mac, uint8_t addrType);
    void close();          // ESC: HEXINPUT -> CHAR_LIST -> voll geschlossen
    void navigateNext();
    void navigatePrev();
    void selectCurrent();  // ENTER: CHAR_LIST -> öffnet Eingabe, HEXINPUT -> sendet
    void appendChar(char c);
    void backspace();
    bool isOpen();
    bool isInputOpen();    // true nur im Freitext-Eingabemodus
    void draw();
    void tick();
}
