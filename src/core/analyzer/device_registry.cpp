#include "device_registry.h"

bool DeviceRegistry::isNewDevice(const std::string& addr)
{
    if (seenDevices.find(addr) != seenDevices.end()) {
        return false;
    }

    seenDevices.insert(addr);
    return true;
}

size_t DeviceRegistry::size() const {
    return seenDevices.size();
}

void DeviceRegistry::clear() {
    seenDevices.clear();
    seenFingerprints.clear();
}

bool DeviceRegistry::isNewFingerprint(const SoftFingerprint& fp)
{
    if (seenFingerprints.find(fp) != seenFingerprints.end()) {
        return false;
    }

    seenFingerprints.insert(fp);
    return true;
}