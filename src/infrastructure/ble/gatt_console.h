#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace GattConsole {
    struct CharInfo {
        std::string uuid;
        bool canWrite;
        bool canRead;
        bool canNotify;
    };

    bool connect(const std::string& mac, uint8_t addrType);
    void disconnectSession();
    bool isConnected();

    std::vector<CharInfo> listWritableChars();

    // Sendet Hex-String ("33 05 0b FF ...") an die UUID, gibt Erfolg zurück
    bool sendHex(const std::string& uuid, const std::string& hexInput);

    // Letzte empfangene Antwort (Notify/Read) als Hex-String
    std::string getLastResponseHex();
}
