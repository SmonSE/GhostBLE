#pragma once

#include <M5Cardputer.h>

class Screenshot {
public:
    static void init();
    static void capture();   // ENTER → snapshot
    static void handle();    // loop → speichern + UI

private:
    static uint16_t* buffer;
    static bool pending;

    static int width;
    static int height;

    static unsigned long messageUntil;
};