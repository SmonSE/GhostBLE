#pragma once

#include <Arduino.h>
#include <cstdint>
#include <string>

// ============================================================
//  ASTM F3411-22a / OpenDroneID Remote ID Parser
//  Source: ASTM F3411-22a, OpenDroneID Core-C, ASD-STAN prEN 4709-002
//
//  BLE Service UUID: 0xFFFA (ASTM Remote ID)
//  Transmitted via: BLE Legacy Advertising (4.x) or Extended (5.x)
//
//  Message types (upper nibble of byte 0):
//    0x0  Basic ID       — drone serial number / registration ID
//    0x1  Location       — GPS position, altitude, speed, heading
//    0x2  Authentication — cryptographic signature (multi-page)
//    0x3  Self-ID        — human-readable description / emergency
//    0x4  System         — operator location, classification
//    0x5  Operator ID    — CAA registration ID
//    0xF  Message Pack   — multiple messages in one (BLE 5 Long Range)
//
//  Protocol versions:
//    0 = ASTM F3411-19
//    1 = ASD-STAN prEN 4709-002
//    2 = ASTM F3411-22a (current)
// ============================================================

// ── Enums ────────────────────────────────────────────────────

enum class DroneIDType {
    None         = 0,
    SerialNumber = 1,  // ANSI/CTA-2063-A Serial Number
    CAA_RegistrationID = 2,
    UTM_AssignedID     = 3,
    SpecificSession    = 4,
};

enum class DroneUAType {
    None              = 0,
    Aeroplane         = 1,
    HelicopterOrMulti = 2,
    Gyroplane         = 3,
    HybridLift        = 4,
    Ornithopter       = 5,
    Glider            = 6,
    Kite              = 7,
    FreeBalloon       = 8,
    CaptiveBalloon    = 9,
    Airship           = 10,
    FreeFallParachute = 11,
    Rocket            = 12,
    TetheredPowered   = 13,
    GroundObstacle    = 14,
    Other             = 15,
};

enum class DroneStatus {
    Undeclared  = 0,
    Ground      = 1,
    Airborne    = 2,
    Emergency   = 3,
    RemoteIDFail = 4,
};

enum class DroneAltitudeRef {
    TakeOff = 0,
    WGS84   = 1,
};

enum class DroneClassEU {
    Undeclared = 0,
    C0 = 1, C1 = 2, C2 = 3, C3 = 4, C4 = 5, C5 = 6, C6 = 7,
};

// ── Message Structs ──────────────────────────────────────────

// Message 0x0 — Basic ID
struct DroneBasicID {
    bool          valid       = false;
    DroneIDType   idType      = DroneIDType::None;
    DroneUAType   uaType      = DroneUAType::None;
    std::string   id;           // serial number or registration ID (20 chars)
};

// Message 0x1 — Location / Vector
struct DroneLocation {
    bool          valid       = false;
    DroneStatus   status      = DroneStatus::Undeclared;
    bool          ewDirection = false;   // true = East, false = West (for speed)
    bool          speedMult   = false;   // speed multiplier flag

    float         latitude    = 0.0f;   // degrees WGS-84
    float         longitude   = 0.0f;   // degrees WGS-84
    float         altitudeBaro = 0.0f;  // meters (barometric)
    float         altitudeGeo  = 0.0f;  // meters (WGS-84)
    float         height       = 0.0f;  // meters above takeoff / ground
    DroneAltitudeRef heightRef = DroneAltitudeRef::TakeOff;

    float         speedH      = 0.0f;   // horizontal speed m/s
    float         speedV      = 0.0f;   // vertical speed m/s
    float         heading     = 0.0f;   // degrees (0–359)

    float         horizAccuracy = 0.0f; // meters
    float         vertAccuracy  = 0.0f; // meters
    float         speedAccuracy = 0.0f; // m/s
    float         tsAccuracy    = 0.0f; // seconds

    uint16_t      timestamp   = 0;      // 1/10s since last hour
};

// Message 0x3 — Self-ID
struct DroneSelfID {
    bool          valid       = false;
    uint8_t       descType    = 0;      // 0=text, 1=emergency, 2=extended status
    std::string   description;          // up to 23 chars
};

// Message 0x4 — System
struct DroneSystem {
    bool          valid            = false;
    bool          operatorLocType  = false;  // 0=takeoff, 1=live GPS
    bool          uaClassification = false;  // 0=undeclared, 1=EU

    float         operatorLat     = 0.0f;   // degrees
    float         operatorLon     = 0.0f;   // degrees
    float         operatorAltGeo  = 0.0f;   // meters

    uint16_t      areaCount       = 0;
    uint8_t       areaRadius      = 0;      // x10 meters
    float         areaCeiling     = 0.0f;   // meters
    float         areaFloor       = 0.0f;   // meters

    DroneClassEU  classEU         = DroneClassEU::Undeclared;
    uint8_t       categoryEU      = 0;

    uint32_t      timestamp       = 0;      // Unix timestamp
};

// Message 0x5 — Operator ID
struct DroneOperatorID {
    bool          valid      = false;
    uint8_t       idType     = 0;
    std::string   operatorId;   // CAA registration ID, up to 20 chars
};

// ── Top-level result ─────────────────────────────────────────

struct DroneIDResult {
    bool          valid      = false;
    uint8_t       protoVersion = 0;

    DroneBasicID    basicId;
    DroneLocation   location;
    DroneSelfID     selfId;
    DroneSystem     system;
    DroneOperatorID operatorId;

    // Convenience helpers
    bool hasGPS()      const { return location.valid && location.latitude  != 0.0f; }
    bool hasOperator() const { return system.valid   && system.operatorLat != 0.0f; }
    bool isEmergency() const { return location.status == DroneStatus::Emergency; }

    // Human-readable summary for logging
    String summary() const;
};

// ── Parser API ───────────────────────────────────────────────

// Parse a single Raw Remote ID BLE service data payload (from 0xFFFA).
// Handles both single messages and message packs (BLE 5).
DroneIDResult parseDroneID(const uint8_t* data, size_t len);

// Convenience: parse from std::string (as returned by NimBLE getServiceData)
DroneIDResult parseDroneIDFromString(const std::string& raw);

// Human-readable UA type string
const char* droneUATypeStr(DroneUAType t);

// Human-readable status string
const char* droneStatusStr(DroneStatus s);
