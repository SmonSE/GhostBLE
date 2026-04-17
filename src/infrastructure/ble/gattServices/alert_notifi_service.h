#pragma once
#include <NimBLEClient.h>
#include <Arduino.h>

class AlertNotificationServiceHandler {
public:
    static String readAlertNotification(NimBLEClient* pClient);
};
