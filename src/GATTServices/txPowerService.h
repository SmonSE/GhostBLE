#pragma once

#include <NimBLEClient.h>
#include <Arduino.h>

class TxPowerServiceHandler {
public:
    static String readTxPowerLevel(NimBLEClient* pClient);
};
