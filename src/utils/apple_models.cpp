#include "apple_models.h"
#include <map>

static std::map<String, String> appleModels = {

    // ============================================================
    // iPhone
    // ============================================================

    // iPhone 8 / X
    {"iPhone10,1", "iPhone 8"},
    {"iPhone10,2", "iPhone 8 Plus"},
    {"iPhone10,3", "iPhone X"},
    {"iPhone10,4", "iPhone 8"},
    {"iPhone10,5", "iPhone 8 Plus"},
    {"iPhone10,6", "iPhone X"},

    // iPhone XS / XR
    {"iPhone11,2", "iPhone XS"},
    {"iPhone11,4", "iPhone XS Max"},
    {"iPhone11,6", "iPhone XS Max"},
    {"iPhone11,8", "iPhone XR"},

    // iPhone 11
    {"iPhone12,1", "iPhone 11"},
    {"iPhone12,3", "iPhone 11 Pro"},
    {"iPhone12,5", "iPhone 11 Pro Max"},
    {"iPhone12,8", "iPhone SE (2nd gen)"},

    // iPhone 12
    {"iPhone13,1", "iPhone 12 mini"},
    {"iPhone13,2", "iPhone 12"},
    {"iPhone13,3", "iPhone 12 Pro"},
    {"iPhone13,4", "iPhone 12 Pro Max"},

    // iPhone 13
    {"iPhone14,2", "iPhone 13 Pro"},
    {"iPhone14,3", "iPhone 13 Pro Max"},
    {"iPhone14,4", "iPhone 13 mini"},
    {"iPhone14,5", "iPhone 13"},
    {"iPhone14,6", "iPhone SE (3rd gen)"},

    // iPhone 14
    {"iPhone14,7", "iPhone 14"},
    {"iPhone14,8", "iPhone 14 Plus"},
    {"iPhone15,2", "iPhone 14 Pro"},
    {"iPhone15,3", "iPhone 14 Pro Max"},

    // iPhone 15
    {"iPhone15,4", "iPhone 15"},
    {"iPhone15,5", "iPhone 15 Plus"},
    {"iPhone16,1", "iPhone 15 Pro"},
    {"iPhone16,2", "iPhone 15 Pro Max"},

    // iPhone 16
    {"iPhone17,1", "iPhone 16 Pro"},
    {"iPhone17,2", "iPhone 16 Pro Max"},
    {"iPhone17,3", "iPhone 16"},
    {"iPhone17,4", "iPhone 16 Plus"},
    {"iPhone17,5", "iPhone 16e"},

    // iPhone 17
    {"iPhone18,1", "iPhone 17"},
    {"iPhone18,2", "iPhone 17 Air"},
    {"iPhone18,3", "iPhone 17 Pro"},
    {"iPhone18,4", "iPhone 17 Pro Max"},
    {"iPhone18,5", "iPhone 17e"},


    // ============================================================
    // iPad
    // ============================================================

    // iPad Air 2
    {"iPad5,3",  "iPad Air 2"},
    {"iPad5,4",  "iPad Air 2"},

    // iPad 7
    {"iPad7,11", "iPad (7th gen)"},
    {"iPad7,12", "iPad (7th gen)"},

    // iPad 8
    {"iPad11,6", "iPad (8th gen)"},
    {"iPad11,7", "iPad (8th gen)"},

    // iPad 9
    {"iPad12,1", "iPad (9th gen)"},
    {"iPad12,2", "iPad (9th gen)"},

    // iPad 10
    {"iPad13,18", "iPad (10th gen)"},
    {"iPad13,19", "iPad (10th gen)"},

    // iPad mini 5
    {"iPad11,1", "iPad mini (5th gen)"},
    {"iPad11,2", "iPad mini (5th gen)"},

    // iPad mini 6
    {"iPad14,1", "iPad mini (6th gen)"},
    {"iPad14,2", "iPad mini (6th gen)"},

    // iPad mini A17 Pro
    {"iPad16,1", "iPad mini (A17 Pro)"},
    {"iPad16,2", "iPad mini (A17 Pro)"},

    // iPad Air 4
    {"iPad13,1", "iPad Air (4th gen)"},
    {"iPad13,2", "iPad Air (4th gen)"},

    // iPad Air 5
    {"iPad13,16", "iPad Air (5th gen)"},
    {"iPad13,17", "iPad Air (5th gen)"},

    // iPad Air 11 / 13 M2
    {"iPad14,8",  "iPad Air 11 (M2)"},
    {"iPad14,9",  "iPad Air 11 (M2)"},
    {"iPad14,10", "iPad Air 13 (M2)"},
    {"iPad14,11", "iPad Air 13 (M2)"},

    // iPad Air M3
    {"iPad15,3", "iPad Air 11 (M3)"},
    {"iPad15,4", "iPad Air 11 (M3)"},
    {"iPad15,5", "iPad Air 13 (M3)"},
    {"iPad15,6", "iPad Air 13 (M3)"},

    // iPad Air M4
    {"iPad16,8",  "iPad Air 11 (M4)"},
    {"iPad16,9",  "iPad Air 11 (M4)"},
    {"iPad16,10", "iPad Air 13 (M4)"},
    {"iPad16,11", "iPad Air 13 (M4)"},

    // iPad Pro 11 M1
    {"iPad13,4", "iPad Pro 11 (3rd gen)"},
    {"iPad13,5", "iPad Pro 11 (3rd gen)"},
    {"iPad13,6", "iPad Pro 11 (3rd gen)"},
    {"iPad13,7", "iPad Pro 11 (3rd gen)"},

    // iPad Pro 12.9 M1
    {"iPad13,8",  "iPad Pro 12.9 (5th gen)"},
    {"iPad13,9",  "iPad Pro 12.9 (5th gen)"},
    {"iPad13,10", "iPad Pro 12.9 (5th gen)"},
    {"iPad13,11", "iPad Pro 12.9 (5th gen)"},

    // iPad Pro 11 M2
    {"iPad14,3", "iPad Pro 11 (4th gen)"},
    {"iPad14,4", "iPad Pro 11 (4th gen)"},

    // iPad Pro 12.9 M2
    {"iPad14,5", "iPad Pro 12.9 (6th gen)"},
    {"iPad14,6", "iPad Pro 12.9 (6th gen)"},

    // iPad Pro 11 M4
    {"iPad16,3", "iPad Pro 11 (M4)"},
    {"iPad16,4", "iPad Pro 11 (M4)"},

    // iPad Pro 13 M4
    {"iPad16,5", "iPad Pro 13 (M4)"},
    {"iPad16,6", "iPad Pro 13 (M4)"},

    // iPad Pro 11 M5
    {"iPad17,1", "iPad Pro 11 (M5)"},
    {"iPad17,2", "iPad Pro 11 (M5)"},

    // iPad Pro 13 M5
    {"iPad17,3", "iPad Pro 13 (M5)"},
    {"iPad17,4", "iPad Pro 13 (M5)"},

    // iPad A16
    {"iPad15,7", "iPad (A16)"},
    {"iPad15,8", "iPad (A16)"},


    // ============================================================
    // Mac
    // ============================================================

    // MacBook Pro M4
    {"Mac16,1", "MacBook Pro 14 (M4)"},

    // MacBook Pro M4 Pro / Max
    {"Mac16,5", "MacBook Pro 16 (M4 Pro/Max)"},
    {"Mac16,6", "MacBook Pro 14 (M4 Pro/Max)"},
    {"Mac16,7", "MacBook Pro 16 (M4 Pro/Max)"},
    {"Mac16,8", "MacBook Pro 14 (M4 Pro/Max)"},

    // Mac mini M4
    {"Mac16,10", "Mac mini (M4)"},
    {"Mac16,11", "Mac mini (M4 Pro)"},

    // MacBook Air M3
    {"Mac15,12", "MacBook Air 15 (M3)"},
    {"Mac15,13", "MacBook Air 13 (M3)"},

    // MacBook Air M4
    {"Mac16,12", "MacBook Air 15 (M4)"},
    {"Mac16,13", "MacBook Air 13 (M4)"},

    // MacBook Air M2
    {"Mac14,2", "MacBook Air 13 (M2)"},

    // MacBook Pro M3
    {"Mac15,6", "MacBook Pro 14 (M3 Pro/Max)"},
    {"Mac15,7", "MacBook Pro 16 (M3 Pro/Max)"},
    {"Mac15,8", "MacBook Pro 14 (M3)"},
    {"Mac15,9", "MacBook Pro 16 (M3)"},

    // MacBook Pro M5
    {"Mac17,1", "MacBook Pro 16 (M5)"},
    {"Mac17,2", "MacBook Pro 14 (M5)"},

    // Mac Studio
    {"Mac13,1", "Mac Studio (M1 Max)"},
    {"Mac13,2", "Mac Studio (M1 Ultra)"},
    {"Mac14,13", "Mac Studio (M2 Max)"},
    {"Mac14,14", "Mac Studio (M2 Ultra)"},
    {"Mac15,14", "Mac Studio (M4 Max)"},
    {"Mac16,9", "Mac Studio (M4 Max)"},

    // iMac
    {"Mac15,4", "iMac 24 (M3)"},
    {"Mac15,5", "iMac 24 (M3)"},

    // Mac mini
    {"Mac14,3", "Mac mini (M2)"},
    {"Mac14,12", "Mac mini (M2 Pro)"},

    // Mac Pro
    {"Mac14,8", "Mac Pro (M2 Ultra)"}
};

bool isAppleModelIdentifier(const String& name) {
    return name.startsWith("iPhone") && name.indexOf(",") != -1;
}

String getAppleModelName(const String& identifier) {
    if (appleModels.count(identifier)) {
        return appleModels[identifier];
    }

    if (identifier.startsWith("iPhone")) return "iPhone (unknown)";
    if (identifier.startsWith("iPad"))   return "iPad (unknown)";
    if (identifier.startsWith("Mac"))    return "Mac (unknown)";

    return identifier;
}