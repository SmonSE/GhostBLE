#pragma once

#include <Arduino.h>
#include "../config/hardware.h"
#include "../config/config.h"
#include "../globals/globals.h"
#include "../logger/logger.h"

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