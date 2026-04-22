#include "notify_handler.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>
#include <vector>
#include <string>

#include "app/context/device_context.h"
#include "app/context/scan_context.h"
#include "infrastructure/logging/logger.h"

// ============================================================
//  Known characteristic UUID → human-readable decoder
//
//  Each entry maps a 16-bit UUID string to a decoder function
//  that turns raw notify bytes into a loggable String.
//  Add new entries here as you discover more devices.
// ============================================================

// ---------------------------------------------------------------------------
//  Decoders for known characteristics
// ---------------------------------------------------------------------------

static int lastCount = 0;

// 0x2A37 — Heart Rate Measurement
// Flags byte: bit0 = value format (0=uint8, 1=uint16)
//             bit2 = energy expended present
//             bit3 = RR-interval present
static String decodeHeartRate(const uint8_t* d, size_t len) {
    if (len < 2) return "Heart rate: invalid payload";
    uint8_t  flags = d[0];
    uint16_t hr    = (flags & 0x01) ? (len >= 3 ? (d[1] | (d[2] << 8)) : 0) : d[1];
    String   out   = "Heart rate: " + String(hr) + " bpm";

    // Optional: RR-interval (ms between beats) — useful for HRV analysis
    if (flags & 0x10) {
        size_t offset = (flags & 0x04) ? 3 : 2;  // skip energy expended if present
        if (offset + 1 < len) {
            uint16_t rr = d[offset] | (d[offset + 1] << 8);
            out += "  RR: " + String(rr) + " ms";
        }
    }
    return out;
}

// 0x2A19 — Battery Level (simple uint8, 0–100%)
static String decodeBattery(const uint8_t* d, size_t len) {
    if (len < 1) return "Battery: invalid payload";
    return "Battery: " + String(d[0]) + "%";
}

// 0x2A6E — Temperature (sint16, unit = 0.01 °C)
static String decodeTemperature(const uint8_t* d, size_t len) {
    if (len < 2) return "Temperature: invalid payload";
    int16_t raw  = (int16_t)(d[0] | (d[1] << 8));
    float   degC = raw / 100.0f;
    return "Temperature: " + String(degC, 2) + " C";
}

// 0x2A1C — Temperature Measurement (IEEE-11073 float, 4 bytes mantissa+exponent)
static String decodeTemperatureMeasurement(const uint8_t* d, size_t len) {
    if (len < 5) return "Temperature measurement: invalid payload";
    // Bytes 1-4: IEEE-11073 FLOAT (exponent in byte4, mantissa in bytes 1-3)
    int32_t mantissa = (int32_t)(d[1] | (d[2] << 8) | (d[3] << 16));
    int8_t  exponent = (int8_t)d[4];
    float   value    = mantissa * powf(10.0f, exponent);
    return "Temperature measurement: " + String(value, 2) + " C";
}

// 0x2A5F — Pulse Oximeter Continuous Measurement
// SpO2 in percent, PR (pulse rate) in bpm
static String decodeSpO2(const uint8_t* d, size_t len) {
    if (len < 3) return "SpO2: invalid payload";
    // Simplified: byte1 = SpO2 %, byte2-3 = pulse rate
    uint8_t  spo2 = d[1];
    uint16_t pr   = d[2] | (d[3] << 8);
    return "SpO2: " + String(spo2) + "%  PR: " + String(pr) + " bpm";
}

// 0x2A56 — Digital (generic I/O, single bit per channel)
static String decodeDigital(const uint8_t* d, size_t len) {
    if (len < 1) return "Digital: invalid payload";
    String out = "Digital I/O:";
    for (size_t i = 0; i < len; i++) {
        out += " 0x" + String(d[i], HEX);
    }
    return out;
}

// 0x2A5B — CSC Measurement (Cycling Speed & Cadence)
static String decodeCSC(const uint8_t* d, size_t len) {
    if (len < 1) return "CSC: invalid payload";
    uint8_t flags = d[0];
    String  out   = "CSC:";
    if ((flags & 0x01) && len >= 7) {
        uint32_t wheelRev = d[1] | (d[2]<<8) | (d[3]<<16) | (d[4]<<24);
        uint16_t lastEvt  = d[5] | (d[6] << 8);
        out += " Wheel rev=" + String(wheelRev) + " evt=" + String(lastEvt);
    }
    if ((flags & 0x02) && len >= 5) {
        size_t   off     = (flags & 0x01) ? 7 : 1;
        uint16_t crankRev = d[off] | (d[off+1] << 8);
        uint16_t lastEvt  = d[off+2] | (d[off+3] << 8);
        out += " Crank rev=" + String(crankRev) + " evt=" + String(lastEvt);
    }
    return out;
}

// 0x2A53 — RSC Measurement (Running Speed & Cadence)
static String decodeRSC(const uint8_t* d, size_t len) {
    if (len < 4) return "RSC: invalid payload";
    uint16_t speed   = d[1] | (d[2] << 8);   // unit: 1/256 m/s
    uint8_t  cadence = d[3];
    float    mps     = speed / 256.0f;
    return "RSC: speed=" + String(mps, 2) + " m/s  cadence=" + String(cadence) + " spm";
}

// 0x2A18 — Glucose Measurement
static String decodeGlucose(const uint8_t* d, size_t len) {
    if (len < 3) return "Glucose: invalid payload";
    uint16_t seqNum = d[1] | (d[2] << 8);
    return "Glucose: seq#" + String(seqNum) + " (raw, " + String(len) + " bytes)";
}

// ---------------------------------------------------------------------------
//  UUID → decoder dispatch table
// ---------------------------------------------------------------------------
struct NotifyDecoder {
    const char* uuid;
    const char* label;
    String (*decode)(const uint8_t*, size_t);
};

static const NotifyDecoder decoders[] = {
    { "2a37", "Heart rate",              decodeHeartRate           },
    { "2a19", "Battery",                 decodeBattery             },
    { "2a6e", "Temperature",             decodeTemperature         },
    { "2a1c", "Temperature measurement", decodeTemperatureMeasurement },
    { "2a5f", "SpO2",                    decodeSpO2                },
    { "2a56", "Digital I/O",             decodeDigital             },
    { "2a5b", "CSC measurement",         decodeCSC                 },
    { "2a53", "RSC measurement",         decodeRSC                 },
    { "2a18", "Glucose measurement",     decodeGlucose             },
};
static constexpr size_t DECODER_COUNT = sizeof(decoders) / sizeof(decoders[0]);

// ---------------------------------------------------------------------------
//  Helper: find decoder for a given UUID string (lowercase, no "0x" prefix)
// ---------------------------------------------------------------------------
static const NotifyDecoder* findDecoder(const std::string& uuid) {
    // NimBLE may return "0x2a37" — strip prefix if present
    std::string clean = uuid;
    if (clean.size() > 2 && clean[0] == '0' && clean[1] == 'x') {
        clean = clean.substr(2);
    }
    for (size_t i = 0; i < DECODER_COUNT; i++) {
        if (clean == decoders[i].uuid) return &decoders[i];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
//  Helper: hex dump fallback for unknown characteristics
// ---------------------------------------------------------------------------
static String hexDump(const uint8_t* d, size_t len) {
    String out;
    out.reserve(len * 3 + 1);
    for (size_t i = 0; i < len; i++) {
        if (d[i] < 0x10) out += "0";
        out += String(d[i], HEX);
        out += " ";
    }
    out.toUpperCase();
    return out;
}

// ============================================================
//  NotifyHandler::subscribeAndCapture
// ============================================================
namespace NotifyHandler {

String subscribeAndCapture(NimBLEClient* pClient, uint32_t captureWindowMs) {
    if (!pClient || !pClient->isConnected()) return "";

    String  summary;
    int     subscribeCount = 0;

    // Collect all characteristics that support notify or indicate
    // across every service on this client.
    std::vector<NimBLERemoteCharacteristic*> targets;

    for (auto svcIt  = pClient->getServices().begin();
              svcIt != pClient->getServices().end(); ++svcIt) {

        NimBLERemoteService* svc = *svcIt;

        for (auto cIt  = svc->getCharacteristics().begin();
                  cIt != svc->getCharacteristics().end(); ++cIt) {

            NimBLERemoteCharacteristic* ch = *cIt;
            if (ch->canNotify() || ch->canIndicate()) {
                targets.push_back(ch);
            }
        }
    }

    if (targets.empty()) {
        return "";  // nothing to subscribe to on this device
    }

    LOG(LOG_NOTIFY, "Subscribing to " + String(targets.size()) + " notifiable characteristic(s)...");

    // Subscribe to all targets.
    // The lambda captures the characteristic pointer so it can log the UUID.
    // We use a shared atomic counter to track total notifications received.
    std::atomic<int> notifyCount{0};

    for (auto* ch : targets) {
        std::string charUuid = ch->getUUID().toString();
        const NotifyDecoder* dec = findDecoder(charUuid);

        bool ok = ch->subscribe(true,
            [charUuid, dec, &notifyCount](
                NimBLERemoteCharacteristic* /*ch*/,
                uint8_t* data,
                size_t   length,
                bool     /*isNotify*/)
            {
                if (length == 0) return;

                String decoded;
                if (dec) {
                    // Known characteristic: human-readable decode
                    decoded = dec->label + String(": ") + dec->decode(data, length);
                    DeviceContext::xpManager.awardXP(2.5f);  // +2.5 XP: known char decoded
                } else {
                    // Unknown: hex dump for manual analysis
                    decoded = "[" + String(charUuid.c_str()) + "] "
                            + hexDump(data, length);
                    DeviceContext::xpManager.awardXP(1.5f);  // +1.5 XP: raw notify data
                }

                LOG(LOG_NOTIFY, "  Notify " + decoded);
                notifyCount.fetch_add(1);
            }
        );

        if (ok) {
            subscribeCount++;
            String label = dec ? String(dec->label) : String(charUuid.c_str());
            LOG(LOG_NOTIFY, "  Subscribed: " + label);
            DeviceContext::xpManager.awardXP(1.0f);  // +1.0 XP: subscription established
        } else {
            LOG(LOG_NOTIFY, "  Subscribe failed: " + String(charUuid.c_str())
                + " (likely requires pairing)");
        }
    }

    if (subscribeCount == 0) {
        return "";  // all subscriptions were rejected — device requires pairing
    }

    // Capture window: block and let notifications arrive.
    // vTaskDelay in 100ms slices so we can bail early if disconnected.
    uint32_t elapsed = 0;
    while (elapsed < captureWindowMs && pClient->isConnected()) {
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed += 100;
    }

    // Unsubscribe cleanly before disconnect
    for (auto* ch : targets) {
        if (ch->canNotify() || ch->canIndicate()) {
            ch->unsubscribe();
        }
    }

    summary = "Captured " + String(notifyCount.load())
            + " notification(s) from " + String(subscribeCount)
            + " characteristic(s)";

    LOG(LOG_NOTIFY, summary);

    lastCount = notifyCount.load();

    return summary;
}

} // namespace NotifyHandler

int NotifyHandler::lastNotifyCount() { return lastCount; }