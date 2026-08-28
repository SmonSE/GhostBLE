#include "exposure_classifier.h"
#include "app/context/globals.h"
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

// Possessive/attribution markers that tend to appear in personal
// device names ("iPhone von Anna", "Anna's iPhone", "S36 Ultra from
// Jens Bauer").
//
// IMPORTANT: this list is inherently incomplete and biased toward the
// locales it was built from (German, English). It will miss the
// equivalent pattern in most other languages. Treat a match here as a
// medium-confidence signal, not a certainty — and extend this list
// deliberately as new locales come up, rather than adding bare
// substring checks (see the bug this replaces, below).
//
// Every marker is padded with spaces so matching requires a word
// boundary on both sides. The previous version had a bare
// `lower.find("von")` check with no boundary, which false-positived on
// any name merely *containing* those letters (e.g. "DevonSpeaker",
// "Avon") — that check has been removed; " von " below already covers
// the real case.
static const std::vector<std::string> personalNameMarkers = {
    " von ",     // German: "iPhone von Anna"
    "'s ",       // English: "Anna's iPhone"
    "\xE2\x80\x99s ", // English (curly apostrophe): "Anna’s iPhone"
    " de ",      // French / Spanish / Portuguese / Italian: "iPhone de Anna"
    " van ",     // Dutch / Afrikaans: "iPhone van Anna"
    " di ",      // Italian: "iPhone di Anna"
    " av ",      // Norwegian / Swedish: "iPhone av Anna"
    " af ",      // Danish: "iPhone af Anna"
    " from ",    // English, weaker/generic: "S36 Ultra from Jens Bauer"
};

bool looksLikePersonalName(const std::string& name)
{
    if (name.length() < 3) return false;

    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Pad with spaces so a marker at the very start/end of the name
    // (unlikely but possible) still matches on a word boundary.
    std::string padded = " " + lower + " ";

    for (const auto& marker : personalNameMarkers) {
        if (padded.find(marker) != std::string::npos) {
            return true;
        }
    }

    // Weak indicators: a known device-type word plus a second word
    // suggests "<Name> <DeviceType>" or "<DeviceType> <Name>" even
    // without a recognized possessive marker. Still English-biased —
    // same caveat as above.
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

    for (const auto& word : roomWords)
    {
        if (lower.find(word) != std::string::npos)
            return true;
    }

    return false;
}

// Extracts the likely owner-name portion following a matched
// possessive/attribution marker (see personalNameMarkers).
//
// Example: "S36 Ultra from Jens Bauer" -> "Jens Bauer"
//          "Anna's iPhone" would need the marker check first — this
//          only extracts what comes AFTER the marker, so it's suited
//          to suffix patterns like Samsung's "<Model> from <Name>",
//          not prefix patterns like "<Name>'s <Model>".
//
// Returns "" if no known marker matched, or if nothing meaningful
// follows the marker. Preserves the original casing of the extracted
// name (matching is done on a lowercased copy, but the substring
// returned is sliced from the untouched input).
std::string extractPossibleOwnerName(const std::string& name)
{
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string padded = " " + lower + " ";

    for (const auto& marker : personalNameMarkers) {
        size_t pos = padded.find(marker);
        if (pos == std::string::npos) continue;

        size_t nameStartPadded = pos + marker.length();
        size_t nameEndPadded = padded.length() - 1; // exclude trailing pad space

        if (nameStartPadded >= nameEndPadded) continue;

        // Map padded indices back onto the original (unpadded,
        // original-case) string — padding added exactly one leading char.
        size_t origStart = nameStartPadded - 1;
        size_t origLen = nameEndPadded - nameStartPadded;

        if (origStart < name.length() && origLen > 0) {
            return name.substr(origStart, origLen);
        }
    }

    return "";
}
