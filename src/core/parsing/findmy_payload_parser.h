// findmy_payload_parser.h
//
// Decodes the Apple "Offline Finding" (Find My) BLE manufacturer-specific
// payload instead of leaving it as a raw hex dump.
//
// Field layout is based on public reverse-engineering work (OpenHaystack,
// AirGuard, and Heinrich/Stute/Hollick "Who Can Find My Devices?", 2021) —
// this is NOT an official Apple specification. The company ID / type /
// length / public-key-split fields are solid and widely corroborated; the
// battery-level interpretation of the final byte is best-effort and should
// be treated as a hint, not a guarantee.
//
// Expected input: the 25 bytes that follow the "4C 00 12 19" AD header
// (i.e. status + 22 key bytes + key-top-bits byte + hint byte).

#pragma once
#include <Arduino.h>
#include <cstdint>
#include <cstring>

#include "utils/string_utils.h"

struct FindMyPayload {
    bool valid = false;
    uint8_t status = 0;
    uint8_t publicKeyPartial[22] = {0};  // bytes 6-27 of the 28-byte public key
    uint8_t publicKeyTopBits = 0;        // top 2 bits of full public key byte 0
    uint8_t hintByte = 0;                // raw last byte, unmodified
    uint8_t batteryLevel = 0;            // best-effort: 0=full,1=medium,2=low,3=critical
};

inline FindMyPayload parseFindMyPayload(const uint8_t* data, size_t len) {
    FindMyPayload result;

    // status(1) + partial pubkey(22) + key-top-bits(1) + hint(1) = 25 bytes
    if (data == nullptr || len < 25) {
        return result;
    }

    result.status = data[0];
    memcpy(result.publicKeyPartial, &data[1], 22);
    result.publicKeyTopBits = (data[23] >> 6) & 0x03;
    result.hintByte = data[24];

    // Best-effort: top 2 bits of the hint byte as battery state.
    // Not confirmed by Apple — treat as a hint, log the raw byte too.
    result.batteryLevel = (data[24] >> 6) & 0x03;

    result.valid = true;
    return result;
}

inline const char* findMyBatteryLabel(uint8_t batteryLevel) {
    switch (batteryLevel) {
        case 0: return "Full";
        case 1: return "Medium";
        case 2: return "Low";
        case 3: return "Critically Low";
        default: return "Unknown";
    }
}

inline String findMyPublicKeyPartialHex(const FindMyPayload& payload) {
    String hex = "";
    char buf[3];
    for (int i = 0; i < 22; i++) {
        snprintf(buf, sizeof(buf), "%02X", payload.publicKeyPartial[i]);
        hex += buf;
    }
    return hex;
}

// One-line summary suitable for dropping straight into suspicious.log
// underneath the existing "Raw data (N bytes): ..." line.
inline String findMyDecodedSummary(const FindMyPayload& payload) {
    if (!payload.valid) {
        return "  (payload too short to decode — expected 25 bytes)";
    }

    String indent = StringUtils::indentFromTag(devTag);
    String out = "";
    out += indent + "  Status byte:     0x" + String(payload.status, HEX) + "\n";
    out += indent + "  Partial pubkey:  " + findMyPublicKeyPartialHex(payload) + " (22 of 28 bytes)\n";
    out += indent + "  Key top bits:    " + String(payload.publicKeyTopBits, BIN) + "\n";
    out += indent + "  Battery (est.):  " + String(findMyBatteryLabel(payload.batteryLevel)) +
           " (raw hint byte 0x" + String(payload.hintByte, HEX) + ", unofficial decode)";
    return out;
}
