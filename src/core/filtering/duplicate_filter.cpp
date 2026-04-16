#include "duplicate_filter.h"

bool DuplicateFilter::isNew(uint32_t fingerprint)
{
    // If already seen → Duplicate
    if (seenFingerprints.find(fingerprint) != seenFingerprints.end()) {
        return false;
    }

    // New → store
    seenFingerprints.insert(fingerprint);
    return true;
}

void DuplicateFilter::reset()
{
    seenFingerprints.clear();
}