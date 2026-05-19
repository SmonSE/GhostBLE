#include "research_mode.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "infrastructure/logging/logger.h"

// Example implementation for BLE protocol research.
// Only use with devices you own or are authorized to test.

namespace ResearchMode {

// ── Stats instance ────────────────────────────────────────────
ResearchStats stats;

// ============================================================
//  Internal helpers
// ============================================================

// XOR checksum of bytes 0–18
static uint8_t goveeXOR(const uint8_t* cmd) {
    uint8_t x = 0;
    for (int i = 0; i < 19; i++) x ^= cmd[i];
    return x;
}

// Build a 20-byte command and write checksum into byte 19
static void buildCmd(uint8_t* out,
                     uint8_t type, uint8_t sub,
                     uint8_t b2 = 0, uint8_t b3 = 0, uint8_t b4 = 0,
                     uint8_t b5 = 0, uint8_t b6 = 0, uint8_t b7 = 0,
                     uint8_t b8 = 0, uint8_t b9 = 0) {
    memset(out, 0, 20);
    out[0] = 0x33;
    out[1] = type;
    out[2] = sub;
    out[3] = b2; out[4] = b3;  out[5] = b4;
    out[6] = b5; out[7] = b6;  out[8] = b7;
    out[9] = b8; out[10] = b9;
    out[19] = goveeXOR(out);
}

// Write a 20-byte command to the Govee control characteristic.
// Returns true on success.
static bool goveeWrite(NimBLEClient* pClient, const uint8_t* cmd20,
                       const String& devTag, const char* label) {
    if (!pClient || !pClient->isConnected()) return false;

    NimBLERemoteCharacteristic* ch = nullptr;

    // Try Bulb-Service (0xFFF6) first
    NimBLERemoteService* bulbSvc = pClient->getService(GOVEE_BULB_SERVICE_UUID);
    if (bulbSvc) {
        ch = bulbSvc->getCharacteristic(GOVEE_BULB_CHAR_UUID);
        if (ch && !ch->canWrite()) ch = nullptr;
    }

    // Fallback: Strip-Service
    if (!ch) {
        NimBLERemoteService* stripSvc = pClient->getService(GOVEE_STRIP_SERVICE_UUID);
        if (stripSvc) {
            ch = stripSvc->getCharacteristic(GOVEE_STRIP_CHAR_UUID);
            if (ch && !ch->canWrite() && !ch->canWriteNoResponse()) ch = nullptr;
        }
    }

    if (!ch) {
        LOG(LOG_TARGET, devTag + "Evil: no writable characteristic found");
        return false;
    }

    bool ok = ch->writeValue(cmd20, 20, false);
    LOG(LOG_TARGET, devTag + String(label)
        + (ok ? " OK" : " FAILED")
        + " [xor=0x" + String(cmd20[19], HEX) + "]");
    return ok;
}

// ── Command builders ─────────────────────────────────────────

// Keep-Alive: 33 01 01 00...00 | xor = 0x33
static void cmdKeepAlive(uint8_t* out) {
    buildCmd(out, 0x01, 0x01);
}

// Power: 33 01 <01|00> 00...00 | xor
static void cmdPower(uint8_t* out, bool on) {
    buildCmd(out, 0x01, on ? 0x01 : 0x00);
}

// Brightness: 33 04 <pct 0-100> 00...00 | xor
static void cmdBrightness(uint8_t* out, uint8_t pct) {
    buildCmd(out, 0x04, pct);
}

// Solid color: 33 05 02 R G B 00 00 FF 7F 00...00 | xor
//   sub = 0x02 → solid color mode (NOT 0x04 which is segment mode!)
//   bytes 8+9 = 0xFF 0x7F are required by H61xx hardware
static void cmdColor(uint8_t* out, uint8_t r, uint8_t g, uint8_t b) {
    memset(out, 0, 20);
    out[0]  = 0x33;
    out[1]  = 0x05;
    out[2]  = 0x02;  // solid color — segment mode (0x04) won't work here
    out[3]  = r;
    out[4]  = g;
    out[5]  = b;
    out[6]  = 0x00;
    out[7]  = 0x00;
    out[8]  = 0xFF;  // required color mode flag for H61xx
    out[9]  = 0x7F;  // required color mode flag for H61xx
    out[19] = goveeXOR(out);
}

// Scene: 33 05 04 <scene_id> 00...00 | xor
// Common scenes: 0x01=sunrise, 0x02=nightlight, 0x07=romantic
static void cmdScene(uint8_t* out, uint8_t sceneId) {
    buildCmd(out, 0x05, 0x04, sceneId);
}

// ============================================================
//  Public API
// ============================================================

bool isGoveeDevice(const String& name) {
    if (name.isEmpty()) return false;
    if (name.startsWith("Govee_"))   return true;
    if (name.startsWith("GV-"))      return true;
    if (name.startsWith("GVH"))      return true;
    if (name.startsWith("H6"))       return true;
    if (name.startsWith("H7"))       return true;
    if (name.startsWith("ihoment_")) return true;
    if (name.startsWith("Minger_"))  return true;
    if (name.startsWith("Ledenet_")) return true;
    return false;
}

bool hasGoveeService(NimBLEClient* pClient) {
    if (!pClient || !pClient->isConnected()) return false;

    // Bulb-Serie: Service 0xFFF6 + Char 18ee2ef5...11 [W]
    NimBLERemoteService* bulbSvc = pClient->getService(GOVEE_BULB_SERVICE_UUID);
    if (bulbSvc) {
        NimBLERemoteCharacteristic* ch = bulbSvc->getCharacteristic(GOVEE_BULB_CHAR_UUID);
        if (ch && ch->canWrite()) return true;
    }

    // Strip-Serie: Service 00010203... + Char 2b11 [RWN]
    NimBLERemoteService* stripSvc = pClient->getService(GOVEE_STRIP_SERVICE_UUID);
    if (stripSvc) {
        NimBLERemoteCharacteristic* ch = stripSvc->getCharacteristic(GOVEE_STRIP_CHAR_UUID);
        if (ch && (ch->canWrite() || ch->canWriteNoResponse())) return true;
    }

    return false;
}

// RGBWW Bulb Color Command — 5 Kanäle statt 3
// 33 05 02 R G B WW CW FF 7F 00...00 | xor
// WW = Warm White (0-255)
// CW = Cold White (0-255)
static void cmdColorRGBWW(uint8_t* out,
                          uint8_t r, uint8_t g, uint8_t b,
                          uint8_t ww = 0, uint8_t cw = 0) {
    memset(out, 0, 20);
    out[0]  = 0x33;
    out[1]  = 0x05;
    out[2]  = 0x02;
    out[3]  = r;
    out[4]  = g;
    out[5]  = b;
    out[6]  = ww;   // Warm White
    out[7]  = cw;   // Cold White
    out[8]  = 0xFF;
    out[9]  = 0x7F;
    out[19] = goveeXOR(out);
}

bool executeInteraction(NimBLEClient* pClient, const String& devTag) {
    stats.goveeDevicesFound++;

    uint8_t cmd[20];

    // Step 1: Keep-Alive — establish session first
    // Without this Govee disconnects within ~500ms
    cmdKeepAlive(cmd);
    if (!goveeWrite(pClient, cmd, devTag, "Keep-Alive")) {
        LOG(LOG_TARGET, devTag + "Research: keep-alive failed — aborting");
        stats.failedValidations++;
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(300));

    // Step 2: Power ON
    cmdPower(cmd, true);
    goveeWrite(pClient, cmd, devTag, "Power ON");
    vTaskDelay(pdMS_TO_TICKS(200));

    // Step 3: Full brightness
    cmdBrightness(cmd, 100);
    goveeWrite(pClient, cmd, devTag, "Brightness 100%");
    vTaskDelay(pdMS_TO_TICKS(200));

    // Step 4: NibBLEs Yellow (#FFEB3B)
    cmdColorRGBWW(cmd, 0xFF, 0xEB, 0x3B, 0, 0);   // reines RGB Yellow
    bool ok = goveeWrite(pClient, cmd, devTag, "Color YELLOW (#FFEB3B)");

    if (!ok) {
        LOG(LOG_TARGET, devTag + "Research: yellow failed — aborting");
        stats.failedValidations++;
        return false;
    }

    LOG(LOG_TARGET, devTag + "SUCCESS! Govee turned NibBLEs Yellow!");
    vTaskDelay(pdMS_TO_TICKS(1500));

    // Step 5: Keep connection alive for the next commands
    cmdKeepAlive(cmd);
    goveeWrite(pClient, cmd, devTag, "Keep-Alive 2");
    vTaskDelay(pdMS_TO_TICKS(200));

    // Step 6: Bonus — romantic scene
    cmdScene(cmd, 0x07);
    goveeWrite(pClient, cmd, devTag, "Scene romantic");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Step 7: Restore white
    cmdColorRGBWW(cmd, 0, 0, 0, 0, 255);
    goveeWrite(pClient, cmd, devTag, "Restore WHITE");

    stats.successfulValidations++;
    stats.lastValidationTime = millis();
    return true;
}

String getStatsString() {
    String s = "RESEARCH MODE STATS:\n";
    s += "   Govee found:  " + String(stats.goveeDevicesFound) + "\n";
    s += "   Successful:   " + String(stats.successfulValidations) + "\n";
    s += "   Failed:       " + String(stats.failedValidations) + "\n";
    if (stats.lastValidationTime > 0) {
        s += "   Last validation:  "
           + String((millis() - stats.lastValidationTime) / 1000) + "s ago";
    }
    return s;
}

void resetStats() {
    stats = ResearchStats{};
}

} // namespace ResearchMode
