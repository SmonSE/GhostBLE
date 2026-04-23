#pragma once

// Minimal Arduino stub for native (googletest) builds.
// Only defines what the pure-logic files actually use.
// Do NOT add hardware-specific stuff here — keep it minimal.

#include <string>
#include <cstdint>
#include <cmath>
#include <cstdlib>

// Arduino String is std::string on native
using String = std::string;

// Arduino integer types (already in <cstdint> but some headers use these names)
using uint8_t  = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;
using int8_t   = std::int8_t;
using int16_t  = std::int16_t;
using int32_t  = std::int32_t;

inline unsigned long millis() { return 0; }
inline long          random(long max) { return std::rand() % max; }
inline long          random(long min, long max) { return min + std::rand() % (max - min); }
