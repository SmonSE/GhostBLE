// apple_continuity_uuids.h
//
// Known Apple Continuity Protocol service/characteristic UUIDs.
// These show up on almost every modern Apple device (iPhone, iPad, Watch, Mac)
// and were previously logged as "Unknown Service" in gatt.log, inflating the
// "Proprietary" service count in the GATT fingerprint even though they're
// well-documented (Nearby Info, AirDrop, Handoff/Continuity, and a related
// Apple companion service).
//
// Drop this next to your GATT scan code and call
// appleContinuityName(uuidStr) wherever you currently print
// "Unknown Service (0x...)".

#pragma once
#include <Arduino.h>
#include <algorithm>
#include <cctype>

struct KnownUuidEntry {
    const char* uuid;   // lowercase, no dashes stripped — full 128-bit form
    const char* name;
};

// 128-bit UUIDs, lowercase, as they appear in your gatt.log output.
static const KnownUuidEntry APPLE_CONTINUITY_UUIDS[] = {
    { "d0611e78-bbb4-4591-a5f8-487910ae4366", "Apple Continuity Protocol (Nearby Info)" },
    { "8667556c-9a37-4c91-84ed-54ee27d90049", "Apple Continuity Protocol (Nearby Info Char)" },
    { "9fa480e0-4967-4542-9390-d343dc5d04ae", "Apple Continuity Protocol (Handoff)" },
    { "af0badb1-5b99-43cd-917a-a77bc549e3cc", "Apple Continuity Protocol (Handoff Char)" },
    { "7905f431-b5ce-4e99-a40f-4b1e122d00d0", "Apple Media Service (AMS)" },
    { "69d1d8f3-45e1-49a8-9821-9bbdfdaad9d9", "Apple Media Service (Remote Command)" },
    { "9fbf120d-6301-42d9-8c58-25e699a21dbd", "Apple Media Service (Entity Update)" },
    { "22eac6e9-24d6-4bb5-be44-b36ace7c7bfb", "Apple Media Service (Entity Attribute)" },
    { "89d3502b-0f36-433a-8ef4-c502ad55f8dc", "Apple Notification Center Service (ANCS)" },
    { "9b3c81d8-57b1-4a8a-b8df-0e56f7ca51c2", "Apple ANCS (Notification Source)" },
    { "2f7cabce-808d-411f-9a0c-bb92ba96c102", "Apple ANCS (Control Point)" },
    { "c6b2f38c-23ab-46d8-a6ab-a3a870bbd5d7", "Apple ANCS (Data Source)" },
};

static const size_t APPLE_CONTINUITY_UUIDS_COUNT =
    sizeof(APPLE_CONTINUITY_UUIDS) / sizeof(APPLE_CONTINUITY_UUIDS[0]);

// Case-insensitive compare so it doesn't matter whether the caller passes
// the UUID upper- or lower-case.
static bool uuidEqualsIgnoreCase(const String& a, const char* b) {
    if ((size_t)a.length() != strlen(b)) return false;
    for (size_t i = 0; i < a.length(); i++) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) {
            return false;
        }
    }
    return true;
}

// Returns a friendly name for a known Apple Continuity/ANCS/AMS UUID,
// or nullptr if the UUID isn't in the table (caller should fall back to
// "Unknown Service" as before).
inline const char* appleContinuityName(const String& uuid) {
    for (size_t i = 0; i < APPLE_CONTINUITY_UUIDS_COUNT; i++) {
        if (uuidEqualsIgnoreCase(uuid, APPLE_CONTINUITY_UUIDS[i].uuid)) {
            return APPLE_CONTINUITY_UUIDS[i].name;
        }
    }
    return nullptr;
}

// Convenience: is this one of the four *service* UUIDs (not characteristics)?
// Useful if you want to keep the "Proprietary: N" count accurate — i.e.
// exclude these from the proprietary tally since they're standard/known,
// even though they're 128-bit vendor UUIDs.
inline bool isKnownAppleContinuityService(const String& uuid) {
    static const char* serviceUuids[] = {
        "d0611e78-bbb4-4591-a5f8-487910ae4366",
        "9fa480e0-4967-4542-9390-d343dc5d04ae",
        "7905f431-b5ce-4e99-a40f-4b1e122d00d0",
        "89d3502b-0f36-433a-8ef4-c502ad55f8dc",
    };
    for (const char* s : serviceUuids) {
        if (uuidEqualsIgnoreCase(uuid, s)) return true;
    }
    return false;
}
