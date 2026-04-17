#pragma once
#include <string>
#include <unordered_set>


class DeviceRegistry {
public:

    size_t size() const;
    bool isNewDevice(const std::string& addr);
    void clear();

private:
    std::unordered_set<std::string> seenDevices;
};