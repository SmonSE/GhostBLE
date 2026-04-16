#pragma once

#include <cstdint>
#include "core/models/BLEAdvertisementData.h"

// Hauptfunktion
uint32_t makeFingerprint(const BLEAdvertisementData& data);

// Helper (optional für Tests / Erweiterung)
uint32_t hashString(const std::string& str);
uint32_t hashCombine(uint32_t h1, uint32_t h2);