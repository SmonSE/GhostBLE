#include "fmdn_parser.h"

namespace FmdnParser {

FmdnResult parse(uint16_t serviceUuid, const uint8_t* data, size_t len) {
    FmdnResult result;

    if (serviceUuid != UUID_EDDYSTONE && serviceUuid != UUID_FAST_PAIR)
        return result;
    if (data == nullptr || len < 1)
        return result;

    const uint8_t frameType = data[0];
    if (frameType != FRAME_NORMAL && frameType != FRAME_UNWANTED)
        return result;  // real Eddystone (0x00/0x10/0x20/0x30) falls through here

    result.detected         = true;
    result.frameType        = frameType;
    result.unwantedTracking = (frameType == FRAME_UNWANTED);
    return result;
}

}  // namespace FmdnParser