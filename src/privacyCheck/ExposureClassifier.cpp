#include "ExposureClassifier.h"
#include "../config/constants.h"
#include <algorithm>
#include <cctype>
#include <vector>

bool looksLikeIdentityData(const std::string& value)
{
    // serial numbers, product IDs etc.
    if (value.length() > 6 && value.length() < 32)
    {
        int digits = 0;
        for (char c : value)
            if (isdigit(c)) digits++;

        // many digits → likely serial / identity info
        return digits > (value.length() / 2);
    }

    return false;
}

bool looksLikePersonalName(const std::string& name)
{
    if (name.length() < 3) return false;

    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Strong indicators
    if (lower.find(" von ") != std::string::npos) return true;
    if (lower.find("'s ") != std::string::npos) return true;
    if (lower.find("’s ") != std::string::npos) return true;
    if (lower.find("von") != std::string::npos) return true;
    if (lower.find("from") != std::string::npos) return true;

    // Weak indicators
    bool deviceWord =
        lower.find("iphone") != std::string::npos ||
        lower.find("ipad") != std::string::npos ||
        lower.find("galaxy") != std::string::npos ||
        lower.find("pixel") != std::string::npos ||
        lower.find("airpods") != std::string::npos ||
        lower.find("smart") != std::string::npos ||
        lower.find("tag") != std::string::npos;

    bool hasSpace = lower.find(" ") != std::string::npos;

    if (deviceWord && hasSpace)
        return true;

    return false;
}

bool looksLikeEnvironmentName(const std::string& name)
{
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    const std::vector<std::string>& roomWords = getRoomWords();

    for (const auto& word : roomWords)
    {
        if (lower.find(word) != std::string::npos)
            return true;
    }

    return false;
}

