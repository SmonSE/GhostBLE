#include "drone_id_parser.h"

#include <cstring>
#include <cstdio>

// ============================================================
//  ASTM F3411-22a message format constants
//
//  Every BLE Legacy Advertising message is exactly 25 bytes:
//    Byte 0:      [msg_type (4 bits)] [proto_version (4 bits)]
//    Bytes 1-24:  message payload (depends on type)
//
//  For Message Pack (0xF):
//    Byte 1:      message size (always 25)
//    Byte 2:      message count (1–9)
//    Bytes 3...:  concatenated messages, each 25 bytes
// ============================================================

static constexpr uint8_t  MSG_SIZE         = 25;
static constexpr uint8_t  MSG_TYPE_BASIC   = 0x0;
static constexpr uint8_t  MSG_TYPE_LOC     = 0x1;
static constexpr uint8_t  MSG_TYPE_AUTH    = 0x2;
static constexpr uint8_t  MSG_TYPE_SELFID  = 0x3;
static constexpr uint8_t  MSG_TYPE_SYSTEM  = 0x4;
static constexpr uint8_t  MSG_TYPE_OPID    = 0x5;
static constexpr uint8_t  MSG_TYPE_PACK    = 0xF;

// ============================================================
//  Decode helpers
// ============================================================

// Signed 32-bit little-endian
static int32_t getInt32LE(const uint8_t* d, int offset) {
    return (int32_t)(
        (uint32_t)d[offset]         |
        ((uint32_t)d[offset+1] << 8)  |
        ((uint32_t)d[offset+2] << 16) |
        ((uint32_t)d[offset+3] << 24)
    );
}

// Unsigned 16-bit little-endian
static uint16_t getUint16LE(const uint8_t* d, int offset) {
    return (uint16_t)(d[offset] | (d[offset+1] << 8));
}

// Unsigned 32-bit little-endian
static uint32_t getUint32LE(const uint8_t* d, int offset) {
    return (uint32_t)(
        d[offset]           |
        ((uint32_t)d[offset+1] << 8)  |
        ((uint32_t)d[offset+2] << 16) |
        ((uint32_t)d[offset+3] << 24)
    );
}

// GPS coordinate: encoded as int32, resolution 1e-7 degrees
static float decodeLatLon(int32_t raw) {
    return raw * 1e-7f;
}

// Altitude: encoded as uint16, offset -1000m, resolution 0.5m
static float decodeAltitude(uint16_t raw) {
    return raw * 0.5f - 1000.0f;
}

// Height: encoded as uint16, resolution 0.5m
static float decodeHeight(uint16_t raw) {
    return raw * 0.5f;
}

// Horizontal speed: encoded as uint8
// If speedMult = 0: resolution 0.25 m/s, max 63.75
// If speedMult = 1: resolution 0.75 m/s, max 254.25 (high speed)
static float decodeSpeedH(uint8_t raw, bool mult) {
    return raw * (mult ? 0.75f : 0.25f);
}

// Vertical speed: encoded as int8, resolution 0.5 m/s
static float decodeSpeedV(int8_t raw) {
    return raw * 0.5f;
}

// Heading: encoded as uint16, resolution 1 degree (0–359)
static float decodeHeading(uint16_t raw) {
    return (float)(raw % 360);
}

// Horizontal accuracy table (F3411-22a Table A1-1)
static float horizAccuracyTable(uint8_t val) {
    switch (val) {
        case 1:  return 18520.0f;   // < 18.52 km (10 NM)
        case 2:  return 7408.0f;    // < 7.408 km
        case 3:  return 3704.0f;    // < 3.704 km
        case 4:  return 1852.0f;    // < 1.852 km
        case 5:  return 926.0f;
        case 6:  return 555.6f;
        case 7:  return 185.2f;
        case 8:  return 92.6f;
        case 9:  return 30.0f;
        case 10: return 10.0f;
        case 11: return 3.0f;
        case 12: return 1.0f;
        default: return 0.0f;       // 0 = unknown
    }
}

// Vertical accuracy table (F3411-22a Table A1-2)
static float vertAccuracyTable(uint8_t val) {
    switch (val) {
        case 1:  return 150.0f;
        case 2:  return 45.0f;
        case 3:  return 25.0f;
        case 4:  return 10.0f;
        case 5:  return 3.0f;
        case 6:  return 1.0f;
        default: return 0.0f;
    }
}

// Speed accuracy table (F3411-22a Table A1-3)
static float speedAccuracyTable(uint8_t val) {
    switch (val) {
        case 1:  return 10.0f;
        case 2:  return 3.0f;
        case 3:  return 1.0f;
        case 4:  return 0.3f;
        default: return 0.0f;
    }
}

// ============================================================
//  Message decoders
// ============================================================

// Message 0x0 — Basic ID
// Payload: [idType|uaType][id 20 bytes][reserved 3 bytes]
static DroneBasicID decodeBasicID(const uint8_t* p) {
    DroneBasicID b;

    b.idType = (DroneIDType)((p[0] >> 4) & 0x0F);
    b.uaType = (DroneUAType)(p[0] & 0x0F);

    // ID: 20 ASCII bytes, null-terminated within that range
    char buf[21] = {};
    memcpy(buf, p + 1, 20);
    buf[20] = '\0';
    b.id = buf;

    // Trim trailing spaces / nulls
    while (!b.id.empty() && (b.id.back() == ' ' || b.id.back() == '\0')) {
        b.id.pop_back();
    }

    b.valid = !b.id.empty();
    return b;
}

// Message 0x1 — Location / Vector
// Payload (24 bytes after msg header byte):
// [status|flags][heading u16][speedH u8][speedV i8]
// [lat i32][lon i32][altBaro u16][altGeo u16]
// [height u16][horizAccuracy|vertAccuracy][speedAccuracy|reserved]
// [timestamp u16][tsReserved|tsAccuracy][reserved 2]
static DroneLocation decodeLocation(const uint8_t* p) {
    DroneLocation l;

    uint8_t statusByte = p[0];
    l.status    = (DroneStatus)((statusByte >> 4) & 0x0F);
    l.ewDirection = (statusByte >> 2) & 0x01;
    l.speedMult   = (statusByte >> 3) & 0x01;
    l.heightRef   = (statusByte & 0x04) ? DroneAltitudeRef::WGS84 : DroneAltitudeRef::TakeOff;

    uint16_t headingRaw = getUint16LE(p, 1);
    l.heading   = decodeHeading(headingRaw);

    l.speedH    = decodeSpeedH(p[3], l.speedMult);
    l.speedV    = decodeSpeedV((int8_t)p[4]);

    int32_t latRaw = getInt32LE(p, 5);
    int32_t lonRaw = getInt32LE(p, 9);
    l.latitude  = decodeLatLon(latRaw);
    l.longitude = decodeLatLon(lonRaw);

    l.altitudeBaro = decodeAltitude(getUint16LE(p, 13));
    l.altitudeGeo  = decodeAltitude(getUint16LE(p, 15));
    l.height       = decodeHeight(getUint16LE(p, 17));

    uint8_t accByte = p[19];
    l.horizAccuracy = horizAccuracyTable((accByte >> 4) & 0x0F);
    l.vertAccuracy  = vertAccuracyTable(accByte & 0x0F);

    uint8_t speedAccByte = p[20];
    l.speedAccuracy = speedAccuracyTable((speedAccByte >> 4) & 0x0F);

    l.timestamp = getUint16LE(p, 21);

    uint8_t tsAccByte = p[23];
    l.tsAccuracy = (tsAccByte & 0x0F) * 0.1f;  // 1/10 seconds

    // Validity: lat/lon must be non-zero and not sentinel values
    l.valid = (latRaw != 0 && lonRaw != 0 &&
               l.latitude  >=  -90.0f && l.latitude  <=  90.0f &&
               l.longitude >= -180.0f && l.longitude <= 180.0f);
    return l;
}

// Message 0x3 — Self-ID
// Payload: [descType][description 23 bytes]
static DroneSelfID decodeSelfID(const uint8_t* p) {
    DroneSelfID s;
    s.descType = p[0];
    char buf[24] = {};
    memcpy(buf, p + 1, 23);
    buf[23] = '\0';
    s.description = buf;
    while (!s.description.empty() &&
           (s.description.back() == ' ' || s.description.back() == '\0')) {
        s.description.pop_back();
    }
    s.valid = true;
    return s;
}

// Message 0x4 — System
// Payload:
// [flags][operatorLat i32][operatorLon i32][areaCount u16]
// [areaRadius u8][areaCeiling u16][areaFloor u16]
// [uaClass|category u8][operatorAltGeo u16][timestamp u32]
static DroneSystem decodeSystem(const uint8_t* p) {
    DroneSystem s;

    uint8_t flags = p[0];
    s.operatorLocType  = (flags >> 0) & 0x01;
    s.uaClassification = (flags >> 1) & 0x01;

    int32_t opLatRaw = getInt32LE(p, 1);
    int32_t opLonRaw = getInt32LE(p, 5);
    s.operatorLat = decodeLatLon(opLatRaw);
    s.operatorLon = decodeLatLon(opLonRaw);

    s.areaCount  = getUint16LE(p, 9);
    s.areaRadius = p[11];   // x10 meters
    s.areaCeiling = decodeAltitude(getUint16LE(p, 12));
    s.areaFloor   = decodeAltitude(getUint16LE(p, 14));

    uint8_t classCat = p[16];
    s.classEU    = (DroneClassEU)((classCat >> 4) & 0x0F);
    s.categoryEU = classCat & 0x0F;

    s.operatorAltGeo = decodeAltitude(getUint16LE(p, 17));
    s.timestamp      = getUint32LE(p, 19);

    s.valid = (opLatRaw != 0 && opLonRaw != 0);
    return s;
}

// Message 0x5 — Operator ID
// Payload: [idType][operatorId 20 bytes][reserved 3 bytes]
static DroneOperatorID decodeOperatorID(const uint8_t* p) {
    DroneOperatorID o;
    o.idType = p[0];
    char buf[21] = {};
    memcpy(buf, p + 1, 20);
    buf[20] = '\0';
    o.operatorId = buf;
    while (!o.operatorId.empty() &&
           (o.operatorId.back() == ' ' || o.operatorId.back() == '\0')) {
        o.operatorId.pop_back();
    }
    o.valid = !o.operatorId.empty();
    return o;
}

// ============================================================
//  Dispatch a single 25-byte message into the result
// ============================================================
static void dispatchMessage(const uint8_t* msg, DroneIDResult& result) {
    uint8_t msgTypeFull = msg[0];
    uint8_t msgType     = (msgTypeFull >> 4) & 0x0F;
    uint8_t protoVer    = msgTypeFull & 0x0F;

    result.protoVersion = protoVer;

    const uint8_t* payload = msg + 1;   // 24 payload bytes follow

    switch (msgType) {
        case MSG_TYPE_BASIC:
            result.basicId = decodeBasicID(payload);
            break;

        case MSG_TYPE_LOC:
            result.location = decodeLocation(payload);
            break;

        case MSG_TYPE_SELFID:
            result.selfId = decodeSelfID(payload);
            break;

        case MSG_TYPE_SYSTEM:
            result.system = decodeSystem(payload);
            break;

        case MSG_TYPE_OPID:
            result.operatorId = decodeOperatorID(payload);
            break;

        case MSG_TYPE_AUTH:
            // Authentication: multi-page signature — skip for now
            // Future: collect pages and verify Ed25519 signature
            break;

        default:
            break;
    }
}

// ============================================================
//  Top-level parser
// ============================================================
DroneIDResult parseDroneID(const uint8_t* data, size_t len) {
    DroneIDResult result;

    if (len < MSG_SIZE) return result;

    uint8_t msgType = (data[0] >> 4) & 0x0F;

    if (msgType == MSG_TYPE_PACK) {
        // Message Pack: byte 1 = msg size, byte 2 = msg count
        // BLE 5 Long Range only — multiple messages concatenated
        if (len < 3) return result;

        uint8_t msgSize  = data[1];
        uint8_t msgCount = data[2];

        if (msgSize != MSG_SIZE) return result;    // unexpected size
        if (msgCount == 0 || msgCount > 9) return result;

        size_t offset = 3;
        for (int i = 0; i < msgCount; i++) {
            if (offset + MSG_SIZE > len) break;
            dispatchMessage(data + offset, result);
            offset += MSG_SIZE;
        }
    } else {
        // Single message
        dispatchMessage(data, result);
    }

    result.valid = result.basicId.valid  ||
                   result.location.valid ||
                   result.selfId.valid   ||
                   result.system.valid   ||
                   result.operatorId.valid;

    return result;
}

DroneIDResult parseDroneIDFromString(const std::string& raw) {
    return parseDroneID(
        reinterpret_cast<const uint8_t*>(raw.data()),
        raw.size()
    );
}

// ============================================================
//  Human-readable helpers
// ============================================================
const char* droneUATypeStr(DroneUAType t) {
    switch (t) {
        case DroneUAType::None:              return "None";
        case DroneUAType::Aeroplane:         return "Aeroplane";
        case DroneUAType::HelicopterOrMulti: return "Helicopter / Multirotor";
        case DroneUAType::Gyroplane:         return "Gyroplane";
        case DroneUAType::HybridLift:        return "Hybrid Lift";
        case DroneUAType::Ornithopter:       return "Ornithopter";
        case DroneUAType::Glider:            return "Glider";
        case DroneUAType::Kite:              return "Kite";
        case DroneUAType::FreeBalloon:       return "Free Balloon";
        case DroneUAType::CaptiveBalloon:    return "Captive Balloon";
        case DroneUAType::Airship:           return "Airship";
        case DroneUAType::FreeFallParachute: return "Free Fall / Parachute";
        case DroneUAType::Rocket:            return "Rocket";
        case DroneUAType::TetheredPowered:   return "Tethered Powered Aircraft";
        case DroneUAType::GroundObstacle:    return "Ground Obstacle";
        default:                             return "Other";
    }
}

const char* droneStatusStr(DroneStatus s) {
    switch (s) {
        case DroneStatus::Ground:        return "On Ground";
        case DroneStatus::Airborne:      return "Airborne";
        case DroneStatus::Emergency:     return "EMERGENCY";
        case DroneStatus::RemoteIDFail:  return "Remote ID System Failure";
        default:                         return "Undeclared";
    }
}

// ============================================================
//  DroneIDResult::summary()
// ============================================================
String DroneIDResult::summary() const {
    if (!valid) return "Invalid Remote ID frame";

    String s = "ASTM F3411 Remote ID (v" + String(protoVersion) + ")\n";

    if (basicId.valid) {
        s += "   Type:        " + String(droneUATypeStr(basicId.uaType)) + "\n";
        s += "   Serial:      " + String(basicId.id.c_str()) + "\n";
    }

    if (operatorId.valid) {
        s += "   Operator ID: " + String(operatorId.operatorId.c_str()) + "\n";
    }

    if (location.valid) {
        s += "   Status:      " + String(droneStatusStr(location.status)) + "\n";
        s += "   Drone GPS:   " + String(location.latitude,  6) + ", "
                                + String(location.longitude, 6) + "\n";
        s += "   Alt (geo):   " + String(location.altitudeGeo,  1) + " m\n";
        s += "   Alt (baro):  " + String(location.altitudeBaro, 1) + " m\n";
        s += "   Height:      " + String(location.height, 1) + " m ("
                                + (location.heightRef == DroneAltitudeRef::TakeOff
                                   ? "above takeoff" : "WGS-84") + ")\n";
        s += "   Speed H:     " + String(location.speedH, 1) + " m/s\n";
        s += "   Speed V:     " + String(location.speedV, 1) + " m/s\n";
        s += "   Heading:     " + String(location.heading, 0) + " deg\n";

        if (location.horizAccuracy > 0)
            s += "   H-Acc:       <" + String(location.horizAccuracy, 0) + " m\n";
    }

    if (system.valid) {
        s += "   Pilot GPS:   " + String(system.operatorLat, 6) + ", "
                                + String(system.operatorLon, 6) + "\n";
        s += "   Pilot Alt:   " + String(system.operatorAltGeo, 1) + " m\n";
        if (system.uaClassification) {
            s += "   EU Class:    C" + String((int)system.classEU) + "\n";
        }
    }

    if (selfId.valid && !selfId.description.empty()) {
        s += "   Description: " + String(selfId.description.c_str()) + "\n";
    }

    return s;
}
