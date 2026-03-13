#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <vector>

// ===== Room Words =====
// Used to detect room-based device names (e.g. "Wohnzimmer Lampe")
inline const std::vector<std::string>& getRoomWords()
{
    static const std::vector<std::string> roomWords = {
        "wohnzimmer", "küche", "kueche", "bad",
        "schlafzimmer", "office", "living",
        "bedroom", "kitchen", "bath",
        "arbeitszimmer", "gästezimmer", "garage", "büro"
    };
    return roomWords;
}

#endif // CONSTANTS_H
