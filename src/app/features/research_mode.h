#ifndef RESEARCH_MODE_H
#define RESEARCH_MODE_H

#include <NimBLEDevice.h>
#include <Arduino.h>

// ===========================================================================
//  Research Mode (Govee Chaos)
//  Detects Govee BLE devices and sends color commands.
//
//  Protocol: Govee H61xx / H62xx series (and rebrands iHoment / Minger)
//  Service:    00010203-0405-0607-0809-0a0b0c0d1910
//  Write char: 00010203-0405-0607-0809-0a0b0c0d2b11  [RWN]
//
//  Command format: 20 bytes
//    Byte 0:     0x33 (command prefix)
//    Byte 1:     command type
//    Byte 2:     sub-command
//    Bytes 3-18: payload (color, brightness, scene etc.)
//    Byte 19:    XOR checksum of bytes 0–18
//
//  Sources: egold555/Govee-Reverse-Engineering,
//           KunaalKumar/Govee-H6072-Reverse-Engineering
// ===========================================================================

namespace ResearchMode {

// Bulb-Serie (H6006, H6009, GVH6006 etc.)
constexpr const char* GOVEE_BULB_SERVICE_UUID = "00010203-0405-0607-0809-0a0b0c0d1910";
constexpr const char* GOVEE_BULB_CHAR_UUID    = "18ee2ef5-263d-4559-959f-4f9c429f9d11";

// Strip-Serie (H6102, H6159 etc.) — bereits vorhanden
constexpr const char* GOVEE_STRIP_SERVICE_UUID = "00010203-0405-0607-0809-0a0b0c0d1910";
constexpr const char* GOVEE_STRIP_CHAR_UUID    = "00010203-0405-0607-0809-0a0b0c0d2b11";

// ── Statistics ───────────────────────────────────────────────
struct ResearchStats {
    uint16_t goveeDevicesFound = 0;
    uint16_t successfulValidations = 0;
    uint16_t failedValidations = 0;
    uint32_t lastValidationTime = 0;
};

extern ResearchStats stats;

// ── Detection ────────────────────────────────────────────────

// Returns true if the device name matches known Govee / rebrand patterns
bool isGoveeDevice(const String& name);

// Returns true if the connected client exposes the Govee control service
bool hasGoveeService(NimBLEClient* pClient);

// ── Interaction ─────────────────────────────────────────────

// Execute the full Research Mode sequence on a connected Govee device.
// Sends Keep-Alive → Power ON → Brightness 100% → NibBLEs Yellow
// → romantic scene → restore white.
// Returns true on success.
bool executeInteraction(NimBLEClient* pClient, const String& devTag);

// ── Stats ─────────────────────────────────────────────────────

// Returns a formatted stats string for logging
String getStatsString();

// Resets all counters
void resetStats();

} // namespace ResearchMode

#endif // RESEARCH_MODE_H
