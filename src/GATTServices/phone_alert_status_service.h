#pragma once
#include <NimBLEClient.h>
#include <Arduino.h>

class PhoneAlertStatusServiceHandler {
public:
    static String readPhoneAlertStatus(NimBLEClient* pClient);
};
