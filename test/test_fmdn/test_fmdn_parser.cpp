#include <gtest/gtest.h>
#include "core/parsing/fmdn_parser.h"

// Novoo frame from real capture: "40 F4 8E 0D ..." under UUID 0xFEAA
TEST(FmdnParser, DetectsNormalFrameUnderEddystone) {
    const uint8_t data[] = {0x40, 0xF4, 0x8E, 0x0D, 0x40};
    auto r = FmdnParser::parse(0xFEAA, data, sizeof(data));
    EXPECT_TRUE(r.detected);
    EXPECT_FALSE(r.unwantedTracking);
}

TEST(FmdnParser, DetectsUnwantedTrackingFrame) {
    const uint8_t data[] = {0x41, 0x01, 0x02};
    auto r = FmdnParser::parse(0xFEAA, data, sizeof(data));
    EXPECT_TRUE(r.detected);
    EXPECT_TRUE(r.unwantedTracking);
}

TEST(FmdnParser, AlsoAcceptsFastPairUuid) {
    const uint8_t data[] = {0x40, 0x00};
    EXPECT_TRUE(FmdnParser::parse(0xFE2C, data, sizeof(data)).detected);
}

TEST(FmdnParser, IgnoresRealEddystoneFrameTypes) {
    const uint8_t data[] = {0x10, 0x00, 0xAA};  // Eddystone-URL
    EXPECT_FALSE(FmdnParser::parse(0xFEAA, data, sizeof(data)).detected);
}

TEST(FmdnParser, IgnoresUnrelatedUuidAndEmpty) {
    const uint8_t data[] = {0x40};
    EXPECT_FALSE(FmdnParser::parse(0xFEED, data, sizeof(data)).detected);
    EXPECT_FALSE(FmdnParser::parse(0xFEAA, nullptr, 0).detected);
}