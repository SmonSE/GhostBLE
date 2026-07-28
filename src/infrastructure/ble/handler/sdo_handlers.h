#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>

struct SdoContext {
    int rssi = 0;
    String mac;
    String name;
    const uint8_t* serviceData = nullptr;
    size_t      serviceDataLen = 0;
};

// ===== Handler Interface =====
class SdoHandlers {
public:
    static void handleDrone(const SdoContext* ctx = nullptr);
    static void handleFido(const SdoContext* ctx = nullptr);
    static void handleMatter(const SdoContext* ctx = nullptr);
    static bool extract16BitUUID(const NimBLEUUID& uuid, uint16_t& out);
};


