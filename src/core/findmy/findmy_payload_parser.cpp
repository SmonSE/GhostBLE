// findmy_payload_parser.cpp

#include "findmy_payload_parser.h"

#include <cstring>

#include "utils/string_utils.h"

FindMyPayload parseFindMyPayload(const uint8_t* data, size_t len)
{
    FindMyPayload result;

    // status(1) + partial public key(22) +
    // key top bits(1) + hint(1) = 25 bytes
    if (data == nullptr || len < 25) {
        return result;
    }

    result.status = data[0];

    memcpy(
        result.publicKeyPartial,
        &data[1],
        sizeof(result.publicKeyPartial)
    );

    result.publicKeyTopBits = (data[23] >> 6) & 0x03;

    result.hintByte = data[24];

    // Best-effort interpretation of the top two bits.
    //
    // This is NOT confirmed by Apple and should only be treated
    // as a hint.
    result.batteryLevel = (data[24] >> 6) & 0x03;

    result.valid = true;

    return result;
}

const char* findMyBatteryLabel(uint8_t batteryLevel)
{
    switch (batteryLevel) {
        case 0:
            return "Full";

        case 1:
            return "Medium";

        case 2:
            return "Low";

        case 3:
            return "Critically Low";

        default:
            return "Unknown";
    }
}

String findMyPublicKeyPartialHex(const FindMyPayload& payload)
{
    String hex;

    char buf[3];

    for (int i = 0; i < 22; ++i) {
        snprintf(
            buf,
            sizeof(buf),
            "%02X",
            payload.publicKeyPartial[i]
        );

        hex += buf;
    }

    return hex;
}

String findMyDecodedSummary(
    const FindMyPayload& payload,
    const String& devTag)
{
    if (!payload.valid) {
        return "  (payload too short to decode — expected 25 bytes)";
    }

    String indent;

    if (!devTag.isEmpty()) {
        indent = StringUtils::indentFromTag(devTag);
    }

    String out;

    out += indent;
    out += "  Status byte:     0x";
    out += String(payload.status, HEX);

    out += "\n";
    out += indent;
    out += "  Partial pubkey:  ";
    out += findMyPublicKeyPartialHex(payload);
    out += " (22 of 28 bytes)";

    out += "\n";
    out += indent;
    out += "  Key top bits:    ";
    out += String(payload.publicKeyTopBits, BIN);

    out += "\n";
    out += indent;
    out += "  Battery (est.):  ";
    out += findMyBatteryLabel(payload.batteryLevel);
    out += " (raw hint byte 0x";
    out += String(payload.hintByte, HEX);
    out += ", unofficial decode)";

    return out;
}
