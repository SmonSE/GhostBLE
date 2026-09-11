#pragma once
#include <string>
#include <cstdint>

namespace GattLogger {

struct SessionInfo {
    bool        active;
    std::string label;
    std::string mac;
    uint32_t    elapsedMs;
    uint32_t    notifyCount;
    uint32_t    changedCount;
    uint16_t    serviceCount;
    uint16_t    charCount;
    bool        connected;
    std::string lastValueUuid;
    std::string lastValueDecoded;
    uint32_t    lastValueMs;
};

void startSession(const std::string& mac, const std::string& label, uint8_t addrType);
void stopSession();
bool isSessionActive();
SessionInfo getSessionInfo();
void logBootMarker();
}
