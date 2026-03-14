#include "SecurityAnalyzer.h"
#include <NimBLEDevice.h>
#include "../logToSerialAndWeb/logger.h"

// Well-known sensitive service UUIDs
static bool isSensitiveService(const std::string& uuid) {
    return uuid == "0x180d" || uuid == "180d" ||   // Heart Rate
           uuid == "0x1810" || uuid == "1810" ||   // Blood Pressure
           uuid == "0x1808" || uuid == "1808" ||   // Glucose
           uuid == "0x1809" || uuid == "1809" ||   // Health Thermometer
           uuid == "0x1812" || uuid == "1812" ||   // HID (keyboard/mouse)
           uuid == "0x1805" || uuid == "1805";     // Current Time
}

// Well-known DFU / firmware update service UUIDs
static bool isDFUService(const std::string& uuid) {
    // Nordic DFU
    if (uuid.find("fe59") != std::string::npos) return true;
    // Legacy Nordic DFU
    if (uuid.find("1530") != std::string::npos &&
        uuid.find("1234") != std::string::npos) return true;
    // ESP32 OTA
    if (uuid.find("d804b643") != std::string::npos) return true;
    // STM DFU
    if (uuid.find("0000fe20") != std::string::npos) return true;
    return false;
}

// Well-known UART/serial service UUIDs
static bool isUARTService(const std::string& uuid) {
    // Nordic UART Service (NUS)
    if (uuid.find("6e400001") != std::string::npos) return true;
    // HM-10/HM-16 serial
    if (uuid.find("ffe0") != std::string::npos) return true;
    if (uuid.find("ffe1") != std::string::npos) return true;
    // Microchip transparent UART
    if (uuid.find("49535343") != std::string::npos) return true;
    return false;
}

SecurityResult analyzeDeviceSecurity(NimBLEClient* pClient, const DeviceInfo& dev) {
    SecurityResult result;

    if (!pClient || !pClient->isConnected()) return result;

    // -------- 1. Connection encryption check --------
    result.connectionEncrypted = pClient->isEncrypted();

    if (!result.connectionEncrypted) {
        result.findings.push_back({
            "MEDIUM", "NO_ENCRYPTION",
            "connection is not encrypted"
        });
    }

    // -------- 2. Iterate services & characteristics --------
    auto services = pClient->getServices();
    for (auto* service : services) {
        if (!service) continue;
        std::string svcUuid = service->getUUID().toString();

        // Check for DFU service
        if (isDFUService(svcUuid)) {
            result.hasDFUService = true;
            result.findings.push_back({
                "HIGH", "DFU_EXPOSED",
                "firmware update service exposed (" + svcUuid + ")"
            });
        }

        // Check for UART/serial service
        if (isUARTService(svcUuid)) {
            result.hasUARTService = true;
            result.findings.push_back({
                "HIGH", "UART_EXPOSED",
                "serial/UART service exposed (" + svcUuid + ")"
            });
        }

        // Check for sensitive service without encryption
        if (isSensitiveService(svcUuid) && !result.connectionEncrypted) {
            result.hasSensitiveServiceUnencrypted = true;
            result.findings.push_back({
                "HIGH", "SENSITIVE_UNENCRYPTED",
                "sensitive service without encryption (" + svcUuid + ")"
            });
        }

        // Analyze characteristics
        auto chars = service->getCharacteristics();
        for (auto* ch : chars) {
            if (!ch) continue;
            result.totalCharCount++;

            bool writable = ch->canWrite() || ch->canWriteNoResponse();
            if (writable) {
                result.writableCharCount++;

                // Writable without encryption is a security risk
                if (!result.connectionEncrypted) {
                    result.hasWritableWithoutAuth = true;
                    std::string charUuid = ch->getUUID().toString();
                    result.findings.push_back({
                        "HIGH", "WRITABLE_NO_AUTH",
                        "writable characteristic without auth (" + charUuid +
                        " in " + svcUuid + ")"
                    });
                }

                // Write-no-response is especially risky (fire-and-forget)
                if (ch->canWriteNoResponse()) {
                    std::string charUuid = ch->getUUID().toString();
                    result.findings.push_back({
                        "MEDIUM", "WRITE_NO_RESPONSE",
                        "write-no-response characteristic (" + charUuid + ")"
                    });
                }
            }
        }
    }

    // -------- 3. Build device fingerprint --------
    result.deviceFingerprint = buildDeviceFingerprint(pClient, dev.name, dev.manufacturer);

    return result;
}

// -------- Advertisement Flags Analysis (AD Type 0x01) --------
void analyzeAdvFlags(const std::vector<uint8_t>& payload, DeviceInfo& dev,
                     std::vector<SecurityFinding>& findings) {
    // Parse AD structures: [length][type][data...]
    size_t i = 0;
    while (i < payload.size()) {
        uint8_t len = payload[i];
        if (len == 0 || i + len >= payload.size()) break;

        uint8_t adType = payload[i + 1];

        if (adType == 0x01 && len >= 2) {
            // AD Type 0x01 = Flags
            uint8_t flags = payload[i + 2];

            bool leLimited      = flags & 0x01;
            bool leGeneral       = flags & 0x02;
            bool brEdrNotSupport = flags & 0x04;
            bool leBrEdrCtrl     = flags & 0x08;
            bool leBrEdrHost     = flags & 0x10;

            if (!brEdrNotSupport) {
                findings.push_back({
                    "INFO", "BR_EDR_SUPPORTED",
                    "device supports BR/EDR (classic Bluetooth)"
                });
            }

            if (leLimited) {
                findings.push_back({
                    "INFO", "LE_LIMITED_DISCOVERABLE",
                    "limited discoverable mode (temporary visibility)"
                });
            }

            if (leBrEdrCtrl || leBrEdrHost) {
                findings.push_back({
                    "LOW", "DUAL_MODE",
                    "dual-mode device (BLE + Classic)"
                });
            }
        }

        i += len + 1;
    }
}

// -------- Device Fingerprint --------
std::string buildDeviceFingerprint(NimBLEClient* pClient,
                                   const std::string& name,
                                   const std::string& manufacturer) {
    std::string fp;

    // Include sorted service UUIDs
    if (pClient && pClient->isConnected()) {
        auto services = pClient->getServices();
        std::vector<std::string> svcUuids;
        for (auto* svc : services) {
            if (svc) svcUuids.push_back(svc->getUUID().toString());
        }
        std::sort(svcUuids.begin(), svcUuids.end());
        for (auto& u : svcUuids) {
            if (!fp.empty()) fp += "|";
            fp += u;
        }
    }

    // Append manufacturer for more specific fingerprint
    if (!manufacturer.empty()) {
        fp += "#" + manufacturer;
    }

    return fp;
}
