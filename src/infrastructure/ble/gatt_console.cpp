#include "gatt_console.h"
#include <NimBLEDevice.h>
#include "app/context/scan_context.h"
#include "infrastructure/ble/ble_scanner.h"

namespace GattConsole {

static NimBLEClient* client_ = nullptr;
static std::string lastResponseHex_;

static std::vector<uint8_t> parseHexInput(const std::string& hexInput) {
    std::vector<uint8_t> out;
    const char* p = hexInput.c_str();
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        out.push_back((uint8_t)strtoul(p, nullptr, 16));
        while (*p && *p != ' ') p++;
    }
    return out;
}

static void onResponse(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    String hex;
    for (size_t i = 0; i < len; i++) { char b[4]; snprintf(b, sizeof(b), "%02X ", data[i]); hex += b; }
    lastResponseHex_ = hex.c_str();
}

bool connect(const std::string& mac, uint8_t addrType) {
    if (ScanContext::bleScanEnabled.load()) stopBleScan();

    // Warten wie bei GattLogger/GattScripter
    uint32_t waited = 0;
    while (ScanContext::scanIsRunning.load() && waited < 5000) {
        vTaskDelay(pdMS_TO_TICKS(100));
        waited += 100;
    }

    NimBLEAddress addr(mac, addrType);
    client_ = NimBLEDevice::createClient();
    client_->setConnectTimeout(5000);

    if (!client_->connect(addr) || !client_->discoverAttributes()) {
        NimBLEDevice::deleteClient(client_);
        client_ = nullptr;
        startBleScan();
        return false;
    }

    // Auf alle notify/indicate-fähigen Chars subscriben, um Antworten zu sehen
    for (auto* svc : client_->getServices())
        for (auto* chr : svc->getCharacteristics())
            if (chr->canNotify() || chr->canIndicate())
                chr->subscribe(true, onResponse);

    return true;
}

std::vector<CharInfo> listWritableChars() {
    std::vector<CharInfo> out;
    if (!client_) return out;

    for (auto* svc : client_->getServices()) {
        for (auto* chr : svc->getCharacteristics()) {
            if (chr->canWrite() || chr->canWriteNoResponse()) {
                out.push_back({chr->getUUID().toString(),
                                true, chr->canRead(), chr->canNotify() || chr->canIndicate()});
            }
        }
    }
    return out;
}

bool sendHex(const std::string& uuid, const std::string& hexInput) {
    if (!client_) return false;

    NimBLEUUID target(uuid);
    NimBLERemoteCharacteristic* found = nullptr;
    for (auto* svc : client_->getServices())
        for (auto* chr : svc->getCharacteristics())
            if (chr->getUUID().equals(target)) found = chr;

    if (!found) return false;

    auto bytes = parseHexInput(hexInput);
    lastResponseHex_.clear();
    return found->writeValue(bytes.data(), bytes.size(), found->canWrite());
}

std::string getLastResponseHex() { return lastResponseHex_; }

bool isConnected() { return client_ && client_->isConnected(); }

void disconnectSession() {
    if (client_) {
        if (client_->isConnected()) client_->disconnect();
        NimBLEDevice::deleteClient(client_);
        client_ = nullptr;
    }
    startBleScan();
}

} // namespace GattConsole
