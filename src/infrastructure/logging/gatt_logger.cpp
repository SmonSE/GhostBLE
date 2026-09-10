#include "gatt_logger.h"
#include <NimBLEDevice.h>
#include <SD.h>
#include <map>
#include <atomic>

#include "app/context/connected_device_context.h"
#include "app/context/scan_context.h"
#include "infrastructure/logging/logger.h"
#include "infrastructure/ble/ble_scanner.h"


namespace GattLogger {

static SemaphoreHandle_t logMutex_ = nullptr;

static std::atomic<bool> sessionActive_{false};
static std::atomic<bool> stopRequested_{false};
static TaskHandle_t      taskHandle_ = nullptr;
static std::string       targetMac_;
static std::string       targetLabel_;
static uint8_t           targetAddrType_ = BLE_ADDR_PUBLIC;

static std::atomic<uint32_t> sessionStartMillis_{0};
static std::atomic<uint32_t> notifyCount_{0};
static std::atomic<uint32_t> changedCount_{0};
static std::atomic<uint16_t> serviceCount_{0};
static std::atomic<uint16_t> charCount_{0};
static std::atomic<bool>     connected_{false};

// Letzter gesehener Rohwert je Char-UUID — Basis fürs Diff-Logging
static std::map<std::string, std::string> lastValues_;

// Forward-Declaration, da LogClientCallbacks weiter oben steht als die Definition
static void writeGattLog(const String& line);

class LogClientCallbacks : public NimBLEClientCallbacks {
    void onDisconnect(NimBLEClient* pClient, int reason) override {
        writeGattLog("Disconnected, reason = " + String(reason) +
                     " (0x" + String(reason, HEX) + ")");
    }
};

static LogClientCallbacks logCallbacks_;

static void writeGattLog(const String& line) {
    if (logMutex_ == nullptr) return;

    if (xSemaphoreTake(logMutex_, pdMS_TO_TICKS(1000)) == pdTRUE) {
        File f = SD.open("/GhostBLE/detailed.log", FILE_APPEND);
        if (f) {
            f.printf("[%lu] %s\n", millis(), line.c_str());
            f.close();
        }
        xSemaphoreGive(logMutex_);
    }
}

static String hexDump(const std::string& data) {
    String out;
    for (uint8_t b : data) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X ", b);
        out += buf;
    }
    return out;
}

static void onNotify(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    std::string uuid = chr->getUUID().toString();
    std::string val((char*)data, len);

    auto it = lastValues_.find(uuid);
    if (it != lastValues_.end() && it->second == val) return;
    lastValues_[uuid] = val;

    notifyCount_.fetch_add(1);   // NEU

    writeGattLog(String(isNotify ? "[NOTIFY] " : "[INDICATE] ") + uuid.c_str() +
                 " (len=" + String(len) + ") = " + hexDump(val));
}

static void sessionTask(void* param) {
    // ── Wait until the main scan is really stopped ──────────────────
    const uint32_t maxWaitMs = 5000;
    uint32_t waited = 0;
    while (ScanContext::scanIsRunning.load() && waited < maxWaitMs) {
        vTaskDelay(pdMS_TO_TICKS(100));
        waited += 100;
    }

    if (ScanContext::scanIsRunning.load()) {
        writeGattLog("Hauptscan reagierte nicht rechtzeitig auf Stop — Session abgebrochen");
        sessionActive_.store(false);
        stopRequested_.store(false);
        taskHandle_ = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    NimBLEAddress addr(targetMac_, targetAddrType_);
    NimBLEClient*  client = NimBLEDevice::createClient();
    client->setClientCallbacks(&logCallbacks_, false);
    client->setConnectTimeout(5000);

    writeGattLog("===== SESSION START: " + String(targetMac_.c_str()) +
                 " (" + String(targetLabel_.c_str()) + ") =====");

    if (!client->connect(addr)) {
        writeGattLog("Connect failed — Session abgebrochen");
        NimBLEDevice::deleteClient(client);
        sessionActive_.store(false);
        startBleScan();          // Hauptscan wieder freigeben
        taskHandle_ = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    uint16_t conn_handle = client->getConnHandle();
    ConnectedLog::startLogging(conn_handle);
    connected_.store(true);              // NEU
    sessionStartMillis_.store(millis()); // NEU

    client->exchangeMTU();
    writeGattLog("Connected, conn_handle=" + String(conn_handle) +
                 ", MTU=" + String(client->getMTU()));

    bool discoveryOk = client->discoverAttributes();
    writeGattLog(String("discoverAttributes() = ") + (discoveryOk ? "OK" : "FAILED"));

    if (client->getServices().empty()) {
        writeGattLog("Keine Services gefunden (evtl. Pairing erforderlich oder Verbindung instabil)");
    }

    // ── Einmaliger Baseline-Dump (Struktur + Initialwerte) ─────────────
    for (auto* svc : client->getServices()) {
        writeGattLog("Service " + String(svc->getUUID().toString().c_str()));

        for (auto* chr : svc->getCharacteristics()) {
            std::string charUuid = chr->getUUID().toString();

            String flags;
            if (chr->canRead())     flags += "R";
            if (chr->canWrite())    flags += "W";
            if (chr->canNotify())   flags += "N";
            if (chr->canIndicate()) flags += "I";

            std::string initialVal;
            if (chr->canRead()) {
                initialVal = chr->readValue();
                lastValues_[charUuid] = initialVal;   // Baseline merken
            }

            writeGattLog("  Char " + String(charUuid.c_str()) + " [" + flags + "]" +
                (initialVal.empty() ? "" :
                    " (len=" + String(initialVal.size()) + ") = " + hexDump(initialVal)));

            if (chr->canNotify() || chr->canIndicate()) {
                chr->subscribe(true, onNotify);
            }
        }
    }

    serviceCount_.store(client->getServices().size());   // NEU
    uint16_t totalChars = 0;
    for (auto* svc : client->getServices()) totalChars += svc->getCharacteristics().size();
    charCount_.store(totalChars);                          // NEU

    writeGattLog("===== Baseline erfasst — logge nur noch Änderungen =====");

    // ── Session läuft, bis STOP LOG gedrückt wird ──────────────────────
    while (!stopRequested_.load() && client->isConnected()) {
        vTaskDelay(pdMS_TO_TICKS(500));

        // Auch nicht-notify-fähige, aber lesbare Chars periodisch prüfen
        // (deckt Werte ab, die sich ohne Notification ändern können)
        for (auto* svc : client->getServices()) {
            for (auto* chr : svc->getCharacteristics()) {
                if (!chr->canRead() || chr->canNotify() || chr->canIndicate()) continue;

                std::string uuid = chr->getUUID().toString();
                std::string val  = chr->readValue();

                auto it = lastValues_.find(uuid);
                if (it != lastValues_.end() && it->second == val) continue;
                lastValues_[uuid] = val;

                changedCount_.fetch_add(1);   // NEU

                writeGattLog("[CHANGED] " + String(uuid.c_str()) +
                             " (len=" + String(val.size()) + ") = " + hexDump(val));
            }
        }
    }

    writeGattLog("===== SESSION END =====");

    if (client->isConnected()) {
        client->disconnect();
    }
    NimBLEDevice::deleteClient(client);

    ConnectedLog::stopLogging(conn_handle);
    lastValues_.clear();
    sessionActive_.store(false);
    stopRequested_.store(false);

    startBleScan();   // Hauptscan wieder freigeben
    taskHandle_ = nullptr;
    vTaskDelete(nullptr);
}

void startSession(const std::string& mac, const std::string& label, uint8_t addrType) {
    if (sessionActive_.load()) return;

    if (logMutex_ == nullptr) logMutex_ = xSemaphoreCreateMutex();
    if (!SD.exists("/GhostBLE")) SD.mkdir("/GhostBLE");

    notifyCount_.store(0);
    changedCount_.store(0);
    serviceCount_.store(0);
    charCount_.store(0);
    connected_.store(false);
    sessionStartMillis_.store(millis());

    targetMac_      = mac;
    targetLabel_    = label;
    targetAddrType_ = addrType;
    stopRequested_.store(false);
    sessionActive_.store(true);

    xTaskCreatePinnedToCore(sessionTask, "GattLogSession", 8192, nullptr, 4, &taskHandle_, 1);
}

SessionInfo getSessionInfo() {
    SessionInfo info;
    info.active       = sessionActive_.load();
    info.label        = targetLabel_;
    info.mac          = targetMac_;
    info.elapsedMs    = info.active ? (millis() - sessionStartMillis_.load()) : 0;
    info.notifyCount  = notifyCount_.load();
    info.changedCount = changedCount_.load();
    info.serviceCount = serviceCount_.load();
    info.charCount    = charCount_.load();
    info.connected    = connected_.load();
    return info;
}

void stopSession()      { stopRequested_.store(true); }
bool isSessionActive()  { return sessionActive_.load(); }

} // namespace GattLogger
