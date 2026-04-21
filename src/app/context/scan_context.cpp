#include "scan_context.h"

#include <Arduino.h>  // ESP.getFreeHeap()

namespace ScanContext {

// ------------------------------------------------------------
//  Scan-Controller
// ------------------------------------------------------------
std::atomic<bool> bleScanEnabled{false};
std::atomic<bool> scanIsRunning{false};

// ------------------------------------------------------------
//  Found devices
// ------------------------------------------------------------
std::unordered_set<std::string> seenDevices;
std::map<std::string, int>      deviceSessionMap;
std::atomic<int>                nextDeviceSessionId{1};

int getOrAssignDeviceId(const std::string& mac) {
    auto it = deviceSessionMap.find(mac);
    if (it != deviceSessionMap.end()) return it->second;
    int id = nextDeviceSessionId++;
    deviceSessionMap[mac] = id;
    return id;
}

std::vector<std::string> uuidList;
std::vector<std::string> nameList;

// ------------------------------------------------------------
//  Counter
// ------------------------------------------------------------
std::atomic<int> allSpottedDevice{0};
std::atomic<int> susDevice{0};
std::atomic<int> leakedCounter{0};
std::atomic<int> rssi{0};
std::atomic<int> beaconsFound{0};
std::atomic<int> pwnbeaconsFound{0};

// ------------------------------------------------------------
//  Securtiy-Scores
// ------------------------------------------------------------
std::atomic<int> riskScore{0};
std::atomic<int> highFindingsCount{0};
std::atomic<int> unencryptedSensitiveCount{0};
std::atomic<int> writableNoAuthCount{0};

// ------------------------------------------------------------
//  Target-Tracking
// ------------------------------------------------------------
bool isTarget   = false;
bool targetFound = false;
std::atomic<int> targetConnects{0};

// ------------------------------------------------------------
//  Connection strings
// ------------------------------------------------------------
std::string addrStr;
String      is_connectable;

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------
void reset() {
    uuidList.clear();
    nameList.clear();
    addrStr.clear();
    is_connectable = "";

    riskScore.store(0);
    highFindingsCount.store(0);
    unencryptedSensitiveCount.store(0);
    writableNoAuthCount.store(0);
    rssi.store(0);

    isTarget   = false;
    targetFound = false;
}

bool reactiveCleanup(size_t maxDevices, uint32_t minFreeHeap) {
    if (seenDevices.empty()) return false;

    const bool tooMany   = seenDevices.size() >= maxDevices;
    const bool heapLow   = ESP.getFreeHeap() < minFreeHeap;

    if (!tooMany && !heapLow) return false;

    // swap-trick gibt auch die reservierte Kapazität frei
    std::unordered_set<std::string>().swap(seenDevices);
    deviceSessionMap.clear();
    return true;
}

} // namespace ScanContext
