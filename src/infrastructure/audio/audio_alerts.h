#pragma once

// ===========================================================================
//  AudioAlerts
//  ---------------------------------------------------------------------
//  Centralizes the M5.Speaker tone sequences that were previously
//  duplicated at each alert site in ble_scanner.cpp / GattConnectionHandler
//  / sdo_handlers.cpp:
//    - Find My tracker detected            (audioSuspicious-gated)
//    - Known/suspicious target via GATT     (audioSuspicious-gated)
//    - Known/suspicious target via adv.only (audioSuspicious-gated)
//    - Meta Ray-Ban glasses detected        (audioEvilMode-gated)
//    - Drone detected (SDO)                 (audioDrone-gated)
//    - Drone detected, single-tone fallback (audioDrone-gated)
//
//  Each play*() call is a no-op if audio is disabled or the relevant
//  MenuController flag is off — callers no longer need to check
//  ms->audioEnabled / ms->audioSuspicious / ms->audioEvilMode themselves.
//
//  Stateless (namespace, not a class) — same style as MetaGlasses::,
//  SdoHandlers::, NotifyHandler:: elsewhere in the codebase.
// ===========================================================================

namespace AudioAlerts {

// Two-tone ping (1200 Hz x2) — Find My / offline-finding tracker detected.
void playFindMyTrackerAlert();

// Three-tone descending alarm (1800 -> 1400 -> 1000 Hz) — known/suspicious
// target confirmed via an active GATT connection.
void playGattTargetAlert();

// Two-tone alarm (1760 Hz x2) — known/suspicious target detected from
// advertisement data alone (no GATT connection made/possible).
void playAdvertisementTargetAlert();

// Four-tone ascending arpeggio (C5-E5-G5-C6) — Meta Ray-Ban glasses
// GATT service confirmed. Gated by audioEvilMode, not audioSuspicious.
void playMetaGlassesAlert();

// Three-tone alarm (2093 -> 1568 -> 2093 Hz), fixed 200ms gaps between
// tones — drone (SDO) detected. Gated by audioDrone.
void playDroneAlert();

// Single-tone (2093 Hz) fallback variant of the drone alert, used where
// only a brief audio cue is wanted. Gated by audioDrone.
void playDroneAlertShort();

}  // namespace AudioAlerts

