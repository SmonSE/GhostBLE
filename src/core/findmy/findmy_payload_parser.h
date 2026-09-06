// findmy_payload_parser.h
//
// Decodes the Apple "Offline Finding" (Find My) BLE manufacturer-specific
// payload instead of leaving it as a raw hex dump.
//
// Field layout is based on public reverse-engineering work (OpenHaystack,
// AirGuard, and Heinrich/Stute/Hollick "Who Can Find My Devices?", 2021).
// This is NOT an official Apple specification.
//
// Expected input: the 25 bytes that follow the "4C 00 12 19" AD header
// (status + 22 key bytes + key-top-bits byte + hint byte).

#pragma once

#include <Arduino.h>
#include <cstdint>

struct FindMyPayload {
    bool valid = false;

    uint8_t status = 0;

    // Bytes 6-27 of the 28-byte public key.
    uint8_t publicKeyPartial[22] = {0};

    // Top 2 bits of full public key byte 0.
    uint8_t publicKeyTopBits = 0;

    // Raw final byte.
    uint8_t hintByte = 0;

    // Best-effort interpretation:
    // 0 = full
    // 1 = medium
    // 2 = low
    // 3 = critical
    uint8_t batteryLevel = 0;
};

// Parse the 25-byte Apple Find My payload.
FindMyPayload parseFindMyPayload(const uint8_t* data, size_t len);

// Convert the best-effort battery value to a human-readable label.
const char* findMyBatteryLabel(uint8_t batteryLevel);

// Return the 22-byte partial public key as uppercase hexadecimal.
String findMyPublicKeyPartialHex(const FindMyPayload& payload);

// Create a formatted decoded Find My summary.
//
// devTag is optional. Passing an empty string keeps the output untagged.
String findMyDecodedSummary(const FindMyPayload& payload, const String& devTag = "");
