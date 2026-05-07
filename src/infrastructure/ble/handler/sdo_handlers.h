#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>

// Optional: Wenn du später Device-Kontext übergeben willst
struct SdoContext {
    int rssi;
    String mac;
    String name;
};

// ===== Handler Interface =====

class SdoHandlers {
public:
    static void handleDrone(const SdoContext* ctx = nullptr);
    static void handleFido(const SdoContext* ctx = nullptr);
    static void handleMatter(const SdoContext* ctx = nullptr);
    static bool extract16BitUUID(const NimBLEUUID& uuid, uint16_t& out);
};


