#pragma once
#include <Arduino.h>

namespace FileManagerView {
    void open();
    void close();
    void navigateNext();
    void navigatePrev();
    void selectCurrent();
    void confirmDelete();
    void cancelAction();
    bool isInConfirmMode();
    bool isOpen();
    void draw();
}
