#pragma once

#include <unordered_set>
#include <cstdint>

class DuplicateFilter {
public:
    // Check if device with this fingerprint is new (not seen before)
    bool isNew(uint32_t fingerprint);

    // Optional reset function to clear seen fingerprints (e.g. on scan restart)
    void reset();

private:
    std::unordered_set<uint32_t> seenFingerprints;
};