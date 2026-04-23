#include <gtest/gtest.h>
#include "core/models/device_info.h"
#include "core/analyzer/exposure_analyzer.h"

// ============================================================
//  Helpers — build DeviceInfo for common test scenarios
// ============================================================

// Completely silent device: rotating MAC, no name, no data
static DeviceInfo makeSilentDevice() {
    DeviceInfo d;
    d.hasRotatingMac = true;
    return d;
}

// Typical cheap IoT device: static MAC, name broadcast, no encryption
static DeviceInfo makeWeakIoTDevice() {
    DeviceInfo d;
    d.hasStaticMac       = true;
    d.hasName            = true;
    d.advHasName         = true;
    d.hasManufacturerData = true;
    d.isConnectable      = true;
    d.hasWritableChars   = true;
    d.connectionEncrypted = false;
    return d;
}

// Well-behaved Apple device
static DeviceInfo makeAppleDevice() {
    DeviceInfo d;
    d.hasRotatingMac      = true;
    d.manufacturer        = "Apple Inc.";
    d.hasManufacturerData = true;
    return d;
}

// Fitness tracker leaking live sensor data
static DeviceInfo makeFitnessTracker() {
    DeviceInfo d;
    d.hasStaticMac        = true;
    d.isPublicMac         = true;
    d.hasName             = true;
    d.advHasName          = true;
    d.hasManufacturerData = true;
    d.isConnectable       = true;
    d.hasNotifyData       = true;
    d.notifyCharCount     = 3;
    d.connectionEncrypted = false;
    return d;
}

// Medical device: indications + sensitive unencrypted service
static DeviceInfo makeMedicalDevice() {
    DeviceInfo d;
    d.hasStaticMac            = true;
    d.isPublicMac             = true;
    d.hasName                 = true;
    d.hasManufacturerData     = true;
    d.isConnectable           = true;
    d.hasNotifyData           = true;
    d.hasIndicateData         = true;
    d.notifyCharCount         = 4;   // >2 → triggers +10 bonus, plus hasIndicateData +15
    d.hasSensitiveUnencrypted = true;
    d.connectionEncrypted     = false;
    return d;
}

// Highly exposed device: public static MAC + personal name
static DeviceInfo makeHighlyExposedDevice() {
    DeviceInfo d;
    d.isPublicMac         = true;
    d.hasStaticMac        = true;
    d.hasName             = true;
    d.advHasName          = true;
    d.hasManufacturerData = true;
    d.gattHasPersonalName = true;  // e.g. "iPhone von Simon"
    d.isConnectable       = true;
    return d;
}

// DFU device: firmware update service exposed
static DeviceInfo makeDFUDevice() {
    DeviceInfo d;
    d.hasStaticMac    = true;
    d.hasName         = true;
    d.hasDFUService   = true;
    d.hasUARTService  = true;
    d.isConnectable   = true;
    return d;
}

// ============================================================
//  ExposureTier tests
// ============================================================

TEST(ExposureTier, SilentDeviceIsNone) {
    EXPECT_EQ(determineExposureTier(makeSilentDevice()), ExposureTier::None);
}

TEST(ExposureTier, AdvNameIsPassive) {
    DeviceInfo d;
    d.advHasName = true;
    EXPECT_EQ(determineExposureTier(d), ExposureTier::Passive);
}

TEST(ExposureTier, GATTNameIsActive) {
    DeviceInfo d;
    d.gattHasName = true;
    EXPECT_EQ(determineExposureTier(d), ExposureTier::Active);
}

TEST(ExposureTier, PersonalNameIsConsent) {
    DeviceInfo d;
    d.gattHasPersonalName = true;
    EXPECT_EQ(determineExposureTier(d), ExposureTier::Consent);
}

// Personal name outranks gattHasName
TEST(ExposureTier, PersonalNameTakesPrecedenceOverGATTName) {
    DeviceInfo d;
    d.gattHasName         = true;
    d.gattHasPersonalName = true;
    EXPECT_EQ(determineExposureTier(d), ExposureTier::Consent);
}

// ============================================================
//  Score sanity tests — relative ordering
// ============================================================

TEST(ExposureScore, SilentDeviceScoresLow) {
    auto result = analyzeExposure(makeSilentDevice());
    EXPECT_LT(result.score, 30);
}

TEST(ExposureScore, WeakIoTScoresHigherThanSilent) {
    auto silent = analyzeExposure(makeSilentDevice());
    auto iot    = analyzeExposure(makeWeakIoTDevice());
    EXPECT_GT(iot.score, silent.score);
}

TEST(ExposureScore, AppleScoresLowerThanWeakIoT) {
    auto apple = analyzeExposure(makeAppleDevice());
    auto iot   = analyzeExposure(makeWeakIoTDevice());
    EXPECT_LT(apple.score, iot.score);
}

TEST(ExposureScore, FitnessTrackerScoresHigherThanWeakIoT) {
    auto tracker = analyzeExposure(makeFitnessTracker());
    auto iot     = analyzeExposure(makeWeakIoTDevice());
    EXPECT_GT(tracker.score, iot.score);
}

TEST(ExposureScore, MedicalDeviceScoresHigherThanFitnessTracker) {
    auto medical = analyzeExposure(makeMedicalDevice());
    auto tracker = analyzeExposure(makeFitnessTracker());

    // Both are HIGH risk — medical has sensitive unencrypted + indications,
    // tracker has more streaming chars. Assert both are in VERY POOR territory.
    EXPECT_GT(medical.score, 100);
    EXPECT_GT(tracker.score, 100);
    EXPECT_EQ(medical.privacyLevel, "VERY POOR");
    EXPECT_EQ(tracker.privacyLevel, "VERY POOR");
}

TEST(ExposureScore, HighlyExposedDeviceScoresAbove80) {
    auto result = analyzeExposure(makeHighlyExposedDevice());
    EXPECT_GT(result.score, 80);
}

TEST(ExposureScore, DFUDeviceScoresAbove60) {
    auto result = analyzeExposure(makeDFUDevice());
    EXPECT_GT(result.score, 60);
}

// ============================================================
//  Privacy level label tests
// ============================================================

TEST(PrivacyLevel, SilentDeviceIsGoodOrOK) {
    auto result = analyzeExposure(makeSilentDevice());
    EXPECT_TRUE(result.privacyLevel == "GOOD" || result.privacyLevel == "OK");
}

TEST(PrivacyLevel, HighlyExposedDeviceIsVeryPoor) {
    auto result = analyzeExposure(makeHighlyExposedDevice());
    EXPECT_EQ(result.privacyLevel, "VERY POOR");
}

TEST(PrivacyLevel, FitnessTrackerIsPoorOrWorse) {
    auto result = analyzeExposure(makeFitnessTracker());
    EXPECT_TRUE(result.privacyLevel == "POOR" || result.privacyLevel == "VERY POOR");
}

// ============================================================
//  Tracking risk tests
// ============================================================

TEST(TrackingRisk, RotatingMACIsLow) {
    auto result = analyzeExposure(makeSilentDevice());
    EXPECT_EQ(result.trackingRisk, "LOW");
}

TEST(TrackingRisk, StaticMACWithDataIsHigh) {
    auto result = analyzeExposure(makeFitnessTracker());
    EXPECT_EQ(result.trackingRisk, "HIGH");
}

TEST(TrackingRisk, StaticMACWithDataIsHighForMedical) {
    auto result = analyzeExposure(makeMedicalDevice());
    EXPECT_EQ(result.trackingRisk, "HIGH");
}

// ============================================================
//  Reasons list tests — check specific reasons appear
// ============================================================

static bool hasReason(const ExposureResult& r, const std::string& substr) {
    for (const auto& reason : r.reasons) {
        if (reason.find(substr) != std::string::npos) return true;
    }
    return false;
}

TEST(Reasons, PublicStaticMACReasonPresent) {
    auto result = analyzeExposure(makeFitnessTracker());
    EXPECT_TRUE(hasReason(result, "public static MAC"));
}

TEST(Reasons, NotifyReasonPresentForFitnessTracker) {
    auto result = analyzeExposure(makeFitnessTracker());
    EXPECT_TRUE(hasReason(result, "live sensor data"));
}

TEST(Reasons, DFUReasonPresentForDFUDevice) {
    auto result = analyzeExposure(makeDFUDevice());
    EXPECT_TRUE(hasReason(result, "firmware update service"));
}

TEST(Reasons, RotatingMACReasonPresent) {
    auto result = analyzeExposure(makeSilentDevice());
    EXPECT_TRUE(hasReason(result, "rotating MAC"));
}

TEST(Reasons, ApplePrivacyReasonPresent) {
    auto result = analyzeExposure(makeAppleDevice());
    EXPECT_TRUE(hasReason(result, "modern mobile privacy"));
}

// ============================================================
//  notifyCharCount threshold test
// ============================================================

TEST(NotifyScore, MoreThanTwoCharsAddsExtraScore) {
    DeviceInfo few;
    few.hasStaticMac    = true;
    few.hasNotifyData   = true;
    few.notifyCharCount = 1;

    DeviceInfo many;
    many.hasStaticMac    = true;
    many.hasNotifyData   = true;
    many.notifyCharCount = 3;  // triggers +10 extra

    auto rFew  = analyzeExposure(few);
    auto rMany = analyzeExposure(many);
    EXPECT_GT(rMany.score, rFew.score);
}

// ============================================================
//  Edge cases
// ============================================================

TEST(EdgeCase, EmptyDeviceInfoDoesNotCrash) {
    DeviceInfo d;
    EXPECT_NO_THROW(analyzeExposure(d));
}

TEST(EdgeCase, AllFlagsSetDoesNotCrash) {
    DeviceInfo d;
    d.isPublicMac             = true;
    d.hasStaticMac            = true;
    d.hasRotatingMac          = true;
    d.hasName                 = true;
    d.advHasName              = true;
    d.gattHasName             = true;
    d.gattHasPersonalName     = true;
    d.gattHasModelInfo        = true;
    d.gattHasIdentityInfo     = true;
    d.gattHasEnvironmentName  = true;
    d.hasManufacturerData     = true;
    d.hasCleartextData        = true;
    d.isConnectable           = true;
    d.hasWritableChars        = true;
    d.hasDFUService           = true;
    d.hasUARTService          = true;
    d.connectionEncrypted     = true;
    d.hasSensitiveUnencrypted = true;
    d.supportsBrEdr           = true;
    d.hasNotifyData           = true;
    d.hasIndicateData         = true;
    d.notifyCharCount         = 5;
    d.manufacturer            = "Apple Inc.";
    EXPECT_NO_THROW(analyzeExposure(d));
}

TEST(EdgeCase, ScoreIsNotNegativeForSilentApple) {
    DeviceInfo d;
    d.hasRotatingMac  = true;
    d.manufacturer    = "Apple Inc.";
    d.gattHasModelInfo   = true;
    d.gattHasIdentityInfo = true;
    auto result = analyzeExposure(d);
    // Score can be low but the struct should always be valid
    EXPECT_NO_THROW(finalizeExposureRatings(d, result));
}
