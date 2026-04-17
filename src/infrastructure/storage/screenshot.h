#pragma once

#include <Arduino.h>
#include "infrastructure/platform/hardware.h"
#include "config/ui_config.h"
#include "app/context/globals.h"
#include "infrastructure/logging/logger.h"

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