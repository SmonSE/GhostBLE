#pragma once

#include <NimBLEClient.h>
#include <NimBLEServer.h>
#include <Arduino.h>
#include "../config/config.h"

struct PwnBeaconInfo {
  bool valid = false;
  uint8_t version = 0;
  uint8_t flags = 0;
  uint16_t pwnd_run = 0;
  uint16_t pwnd_tot = 0;
  uint8_t fingerprint[PWNBEACON_FINGERPRINT_LEN] = {0};
  String name;
  String face;
  String identity;
  String gattName;
};

class PwnBeaconServiceHandler {
public:
    // === Client (scanning) ===

    // Parse PwnBeacon from advertisement service data
    static PwnBeaconInfo parseAdvertisement(const uint8_t* data, size_t len);

    // Read full PwnBeacon GATT characteristics (face, identity, name)
    static void readGATT(NimBLEClient* pClient, PwnBeaconInfo& info);

    // Format fingerprint as colon-separated hex string
    static String fingerprintToString(const uint8_t fingerprint[PWNBEACON_FINGERPRINT_LEN]);

    // === Server (advertising) ===

    // Start advertising as a PwnBeacon device
    static void startAdvertising(const String& deviceName, const String& face);

    // Update pwnd counters in advertisement data
    static void updateCounters(uint16_t pwndRun, uint16_t pwndTot);

    // Stop advertising
    static void stopAdvertising();
};
