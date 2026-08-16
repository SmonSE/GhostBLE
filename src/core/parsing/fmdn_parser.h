#pragma once

#include <cstdint>
#include <cstddef>

// ===========================================================================
//  Google Find My Device network (FMDN) advertising detection.
//
//  Non-Apple trackers (Motorola Moto Tag, Novoo, Chipolo, Pebblebee) broadcast
//  in Google's Find My Device network. Google documents this under the Fast Pair
//  service UUID 0xFE2C, but real devices (verified by capture) send the frame
//  under the Eddystone UUID 0xFEAA instead — 0xFE2C only appears as a GATT
//  service on connect. The frame starts with a frame-type byte:
//      0x40 = normal (owner nearby)
//      0x41 = unwanted-tracking mode (separated from owner — anti-stalking)
//  followed by a rotating ~20-byte ephemeral ID.
//
//  Collision-free with real Eddystone: its frame types are 0x00/0x10/0x20/0x30,
//  never 0x40/0x41. Purely passive — no connection required.
// ===========================================================================

namespace FmdnParser {

constexpr uint16_t UUID_EDDYSTONE = 0xFEAA;  // observed FMDN broadcast slot
constexpr uint16_t UUID_FAST_PAIR = 0xFE2C;  // documented FMDN slot
constexpr uint8_t  FRAME_NORMAL   = 0x40;
constexpr uint8_t  FRAME_UNWANTED = 0x41;    // separated tracker (stalking-relevant)

struct FmdnResult {
    bool    detected         = false;
    bool    unwantedTracking = false;  // frame type 0x41
    uint8_t frameType        = 0;
};

// serviceUuid: the 16-bit service-data UUID; data/len: the raw payload bytes.
FmdnResult parse(uint16_t serviceUuid, const uint8_t* data, size_t len);

}  // namespace FmdnParser