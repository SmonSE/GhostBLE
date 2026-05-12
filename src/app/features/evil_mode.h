#ifndef EVIL_MODE_H
#define EVIL_MODE_H

#include <NimBLEDevice.h>
#include <Arduino.h>

// ===========================================================================
//  Evil Mode (Govee Chaos)
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

namespace EvilMode {

// ── UUIDs ────────────────────────────────────────────────────
constexpr const char* GOVEE_SERVICE_UUID = "00010203-0405-0607-0809-0a0b0c0d1910";
constexpr const char* GOVEE_CHAR_UUID    = "00010203-0405-0607-0809-0a0b0c0d2b11";

// ── Statistics ───────────────────────────────────────────────
struct EvilStats {
    uint16_t goveeDevicesFound = 0;
    uint16_t successfulAttacks = 0;
    uint16_t failedAttacks     = 0;
    uint32_t lastAttackTime    = 0;
};

extern EvilStats stats;

// ── Detection ────────────────────────────────────────────────

// Returns true if the device name matches known Govee / rebrand patterns
bool isGoveeDevice(const String& name);

// Returns true if the connected client exposes the Govee control service
bool hasGoveeService(NimBLEClient* pClient);

// ── Attack ───────────────────────────────────────────────────

// Execute the full Evil Mode sequence on a connected Govee device.
// Sends Keep-Alive → Power ON → Brightness 100% → NibBLEs Yellow
// → romantic scene → restore white.
// Returns true on success.
bool executeAttack(NimBLEClient* pClient, const String& devTag);

// ── Stats ─────────────────────────────────────────────────────

// Returns a formatted stats string for logging
String getStatsString();

// Resets all counters
void resetStats();

} // namespace EvilMode

#endif // EVIL_MODE_H
