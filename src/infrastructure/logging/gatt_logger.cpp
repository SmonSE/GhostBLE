#include "gatt_logger.h"
#include <NimBLEDevice.h>
#include <SD.h>
#include <map>
#include <atomic>
#include <cstring>

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

static std::map<std::string, std::string> lastValues_;

// ── Notify-Queue-Item (Callback -> Consumer-Task) ──────────────────
struct NotifyLogItem {
    char     uuid[37];
    uint8_t  data[247];   // max. ATT payload bei MTU 247
    size_t   len;
    bool     isNotify;
};

static QueueHandle_t notifyQueue_ = nullptr;
static TaskHandle_t  logConsumerTask_ = nullptr;

static constexpr uint32_t MIN_LOG_INTERVAL_MS = 200;
static std::map<std::string, uint32_t> lastLogTime_;

static void writeGattLog(const String& line);

class LogClientCallbacks : public NimBLEClientCallbacks {
    void onDisconnect(NimBLEClient* pClient, int reason) override {
        connected_.store(false);   // NEU — sofort, nicht erst am Session-Ende
        writeGattLog("Disconnected, reason = " + String(reason) +
                     " (0x" + String(reason, HEX) + ")");
    }
};

static SemaphoreHandle_t lastValueMutex_ = nullptr;
static std::string lastValueUuid_;
static std::string lastValueDecoded_;
static std::atomic<uint32_t> lastValueMs_{0};

static void updateLastDisplayValue(const std::string& uuid, const std::string& decoded) {
    if (lastValueMutex_ == nullptr) return;
    if (xSemaphoreTake(lastValueMutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        lastValueUuid_    = uuid;
        lastValueDecoded_ = decoded;
        lastValueMs_.store(millis());
        xSemaphoreGive(lastValueMutex_);
    }
}

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

static bool tryParseUuid16(const std::string& uuidStr, uint16_t& out) {
    if (uuidStr.size() > 2 && uuidStr[0] == '0' && (uuidStr[1] == 'x' || uuidStr[1] == 'X')) {
        out = (uint16_t)strtoul(uuidStr.c_str() + 2, nullptr, 16);
        return true;
    }
    return false;
}

static String decodeKnownChar(uint16_t uuid16, const std::string& v) {
    const uint8_t* d = (const uint8_t*)v.data();
    size_t len = v.size();

    switch (uuid16) {
        case 0x2A19:  // Battery Level
            if (len >= 1) return String(d[0]) + "%";
            break;

        case 0x2A37: {
            if (len < 2) break;
            bool contact  = d[0] & 0x02;
            bool wide     = d[0] & 0x01;   // 0 = UINT8, 1 = UINT16
            uint16_t hr   = wide && len >= 3 ? (d[1] | (d[2] << 8)) : d[1];
            return String(hr) + " bpm, contact=" + (contact ? "yes" : "no");
        }

        case 0x2A6E:
            if (len >= 2) {
                int16_t raw = (int16_t)(d[0] | (d[1] << 8));
                return String(raw / 100.0f, 2) + " degC";
            }
            break;

        case 0x2A6F:  // Humidity (uint16 * 0.01 %)
            if (len >= 2) {
                uint16_t raw = d[0] | (d[1] << 8);
                return String(raw / 100.0f, 2) + " %RH";
            }
            break;

        case 0x2A6D:  // Pressure (uint32 * 0.1 Pa)
            if (len >= 4) {
                uint32_t raw = d[0] | (d[1] << 8) | (d[2] << 16) | ((uint32_t)d[3] << 24);
                return String(raw / 10.0f, 1) + " Pa";
            }
            break;
    }
    return "";
}

static String decodeGeneric(const std::string& v) {
    const uint8_t* d = (const uint8_t*)v.data();
    size_t len = v.size();
    if (len == 0) return "";

    String out;
    out += "u8=" + String(d[0]) + " i8=" + String((int8_t)d[0]);

    if (len >= 2) {
        uint16_t u16 = d[0] | (d[1] << 8);
        out += " u16=" + String(u16) + " i16=" + String((int16_t)u16);
    }

    if (len >= 4) {
        uint32_t u32 = d[0] | (d[1] << 8) | (d[2] << 16) | ((uint32_t)d[3] << 24);
        float f32;
        memcpy(&f32, &u32, sizeof(f32));
        out += " u32=" + String(u32) + " f32=" + String(f32, 3);
    }

    bool printable = true;
    for (uint8_t b : v) { if (b < 32 || b > 126) { printable = false; break; } }
    if (printable) out += " ascii=\"" + String(v.c_str()) + "\"";

    return out;
}

static String decodeValue(const std::string& uuid, const std::string& v) {
    uint16_t uuid16;
    if (tryParseUuid16(uuid, uuid16)) {
        String known = decodeKnownChar(uuid16, v);
        if (!known.isEmpty()) return known;
    }
    return decodeGeneric(v);
}

static void ensureLogInfra() {
    if (logMutex_ == nullptr) logMutex_ = xSemaphoreCreateMutex();
    if (lastValueMutex_ == nullptr) lastValueMutex_ = xSemaphoreCreateMutex();
    if (!SD.exists("/GhostBLE")) SD.mkdir("/GhostBLE");
}

void logBootMarker() {
    ensureLogInfra();

    writeGattLog("");
    writeGattLog("==================================================");
    writeGattLog("==== [BOOT] NEW BOOT [/GhostBLE/detailed.log] ====");
    writeGattLog("==================================================");
    writeGattLog("");
}

// ── Notify-Callback (NimBLE-Thread) → Queue → Consumer-Task ─────────────
static void onNotify(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    if (notifyQueue_ == nullptr) return;

    NotifyLogItem item{};
    std::string uuid = chr->getUUID().toString();
    strncpy(item.uuid, uuid.c_str(), sizeof(item.uuid) - 1);
    item.len = std::min(len, sizeof(item.data));
    memcpy(item.data, data, item.len);
    item.isNotify = isNotify;

    xQueueSend(notifyQueue_, &item, 0);
}

// ── Consumer-Task: read Notify-Queue, Diff-Check, Drosselung, Logging ─────────────
static void logConsumerTaskFn(void* param) {
    NotifyLogItem item;

    while (true) {
        if (xQueueReceive(notifyQueue_, &item, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        std::string uuid(item.uuid);
        std::string val((char*)item.data, item.len);

        auto it = lastValues_.find(uuid);
        if (it != lastValues_.end() && it->second == val) continue;
        lastValues_[uuid] = val;

        uint32_t now = millis();
        auto lt = lastLogTime_.find(uuid);
        if (lt != lastLogTime_.end() && (now - lt->second) < MIN_LOG_INTERVAL_MS) {
            continue;
        }
        lastLogTime_[uuid] = now;

        notifyCount_.fetch_add(1);

        String decoded = decodeValue(uuid, val);

        writeGattLog(String(item.isNotify ? "[NOTIFY] " : "[INDICATE] ") + uuid.c_str() +
                    " (len=" + String(item.len) + ") = " + hexDump(val) +
                    " {" + decoded + "}");

        updateLastDisplayValue(uuid, std::string(decoded.c_str()));
    }
}

static void sessionTask(void* param) {
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
        startBleScan();
        taskHandle_ = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    uint16_t conn_handle = client->getConnHandle();
    ConnectedLog::startLogging(conn_handle);
    connected_.store(true);
    sessionStartMillis_.store(millis());

    client->exchangeMTU();
    writeGattLog("Connected, conn_handle=" + String(conn_handle) +
                 ", MTU=" + String(client->getMTU()));

    bool discoveryOk = client->discoverAttributes();
    writeGattLog(String("discoverAttributes() = ") + (discoveryOk ? "OK" : "FAILED"));

    if (client->getServices().empty()) {
        writeGattLog("Keine Services gefunden (evtl. Pairing erforderlich oder Verbindung instabil)");
    }

    //    Discovery-Loop ─────────────────────────────
    std::vector<NimBLERemoteCharacteristic*> toSubscribe;

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
                    " (len=" + String(initialVal.size()) + ") = " + hexDump(initialVal) +
                    " {" + decodeValue(charUuid, initialVal) + "}"));        

            if (chr->canNotify() || chr->canIndicate()) {
                toSubscribe.push_back(chr);
            }
        }
    }

    serviceCount_.store(client->getServices().size());
    uint16_t totalChars = 0;
    for (auto* svc : client->getServices()) totalChars += svc->getCharacteristics().size();
    charCount_.store(totalChars);

    writeGattLog("===== Baseline erfasst — logge nur noch Änderungen =====");

    notifyQueue_ = xQueueCreate(32, sizeof(NotifyLogItem));
    lastLogTime_.clear();
    xTaskCreatePinnedToCore(logConsumerTaskFn, "GattLogConsumer", 6144, nullptr, 3, &logConsumerTask_, 1);

    for (auto* chr : toSubscribe) {
        chr->subscribe(true, onNotify);
    }

    while (!stopRequested_.load() && client->isConnected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (!client->isConnected()) break;

        for (auto* svc : client->getServices()) {
            if (!client->isConnected()) break;

            for (auto* chr : svc->getCharacteristics()) {
                if (!chr->canRead() || chr->canNotify() || chr->canIndicate()) continue;
                if (!client->isConnected()) break;

                std::string uuid = chr->getUUID().toString();
                std::string val  = chr->readValue();

                auto it = lastValues_.find(uuid);
                bool wasNonEmpty = (it != lastValues_.end() && !it->second.empty());

                if (val.empty() && wasNonEmpty) continue;

                if (it != lastValues_.end() && it->second == val) continue;
                lastValues_[uuid] = val;
                changedCount_.fetch_add(1);

                String decoded = decodeValue(uuid, val);

                writeGattLog("[CHANGED] " + String(uuid.c_str()) +
                            " (len=" + String(val.size()) + ") = " + hexDump(val) +
                            " {" + decoded + "}");

                updateLastDisplayValue(uuid, std::string(decoded.c_str()));
            }
        }
    }

    writeGattLog("===== SESSION END =====");

    if (client->isConnected()) {
        client->disconnect();
    }
    NimBLEDevice::deleteClient(client);

    if (logConsumerTask_ != nullptr) {
        vTaskDelete(logConsumerTask_);
        logConsumerTask_ = nullptr;
    }
    if (notifyQueue_ != nullptr) {
        vQueueDelete(notifyQueue_);
        notifyQueue_ = nullptr;
    }

    ConnectedLog::stopLogging(conn_handle);
    lastValues_.clear();
    lastLogTime_.clear();
    sessionActive_.store(false);
    stopRequested_.store(false);
    connected_.store(false);

    startBleScan();
    taskHandle_ = nullptr;
    vTaskDelete(nullptr);
}

void startSession(const std::string& mac, const std::string& label, uint8_t addrType) {
    if (sessionActive_.load()) return;

    lastValueUuid_.clear();
    lastValueDecoded_.clear();
    lastValueMs_.store(0);

    ensureLogInfra();

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

    if (lastValueMutex_ != nullptr &&
        xSemaphoreTake(lastValueMutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        info.lastValueUuid    = lastValueUuid_;
        info.lastValueDecoded = lastValueDecoded_;
        info.lastValueMs      = lastValueMs_.load();
        xSemaphoreGive(lastValueMutex_);
    }

    return info;
}

void stopSession()      { stopRequested_.store(true); }
bool isSessionActive()  { return sessionActive_.load(); }

} // namespace GattLogger
