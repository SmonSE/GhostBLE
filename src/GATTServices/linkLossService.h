#pragma once

#include <NimBLEClient.h>
#include <Arduino.h>

class LinkLossServiceHandler {
public:
    static String readLinkLoss(NimBLEClient* pClient);
};
