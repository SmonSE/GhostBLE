#include "apple_models.h"
#include <map>

static std::map<String, String> appleModels = {

    // iPhone
    {"iPhone12,1", "iPhone 11"},
    {"iPhone12,8", "iPhone SE"},
    {"iPhone13,2", "iPhone 12"},
    {"iPhone14,2", "iPhone 13 Pro"},
    {"iPhone14,5", "iPhone 13"},
    {"iPhone15,2", "iPhone 14 Pro"},
    {"iPhone15,3", "iPhone 14 Pro Max"},
    {"iPhone17,3", "iPhone 16"},
    {"iPhone17,4", "iPhone 16 Pro"},
    {"iPhone17,5", "iPhone 16 Pro Max"},

    // iPad
    {"iPad5,3", "iPad Air 2"},  // wifi
    {"iPad5,4", "iPad Air 2"},  // cellular
    {"iPad7,11",  "iPad (7th gen)"},
    {"iPad7,12",  "iPad (7th gen)"},
    {"iPad11,6",  "iPad (8th gen)"},
    {"iPad11,7",  "iPad (8th gen)"},
    {"iPad12,1",  "iPad (9th gen)"},
    {"iPad12,2",  "iPad (9th gen)"},
    {"iPad13,4",  "iPad Pro 11 (3rd gen)"},
    {"iPad13,5",  "iPad Pro 11 (3rd gen)"},
    {"iPad13,6",  "iPad Pro 11 (3rd gen)"},
    {"iPad13,7",  "iPad Pro 11 (3rd gen)"},
    {"iPad14,1",  "iPad mini (6th gen)"},
    {"iPad14,2",  "iPad mini (6th gen)"},

    // Mac
    {"Mac16,1",   "MacBook Pro 16"},
    {"Mac16,2",   "MacBook Pro 14"},
    {"Mac17,1",   "MacBook Air 15"},
    {"Mac17,2",   "MacBook Air 13"},
    {"Mac18,3",   "MacBook Pro 14"},
    {"Mac18,4",   "MacBook Pro 16"},
    {"Mac19,1",   "MacBook Pro 14"},
    {"Mac19,2",   "MacBook Pro 16"}
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