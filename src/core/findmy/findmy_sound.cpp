#include "findmy_sound.h"

#include <NimBLEDevice.h>

#include "infrastructure/logging/logger.h"

namespace FindMySound {
namespace {

// ============================================================
// Tracker types
// ============================================================

static constexpr uint8_t TYPE_AIRTAG = 0;
static constexpr uint8_t TYPE_FMNA   = 1;
static constexpr uint8_t TYPE_DULT   = 2;

// ============================================================
// Apple AirTag legacy sound control
// ============================================================

static const NimBLEUUID AIRTAG_SERVICE_UUID(
    "7dfc9000-7d1c-4951-86aa-8d9728f8d66c"
);

static const NimBLEUUID AIRTAG_CHARACTERISTIC_UUID(
    "7dfc9001-7d1c-4951-86aa-8d9728f8d66c"
);

static constexpr uint8_t AIRTAG_BEEP_COMMAND = 0xAF;

// ============================================================
// Find My Network Accessory (FMNA)
// ============================================================

static const NimBLEUUID FMNA_SERVICE_UUID(
    "0000fd44-0000-1000-8000-00805f9b34fb"
);

static const NimBLEUUID FMNA_SOUND_CHARACTERISTIC_UUID(
    "4f860003-943b-49ef-bed4-2f730304427a"
);

static const uint8_t FMNA_START_SOUND_COMMAND[] = {
    0x01, 0x00, 0x03
};

// ============================================================
// DULT
// ============================================================

static const NimBLEUUID DULT_SERVICE_UUID(
    "15190001-12f4-c226-88ed-2ac5579f2a85"
);

static const NimBLEUUID DULT_SOUND_CHARACTERISTIC_UUID(
    "8e0c0001-1d68-fb92-bf61-48377421680e"
);

static const uint8_t DULT_START_SOUND_COMMAND[] = {
    0x00, 0x03
};

// ============================================================
// Connection configuration
// ============================================================

static constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;

// ============================================================
// Notification / indication callback
// ============================================================

void trackerNotifyCallback(
    NimBLERemoteCharacteristic* characteristic,
    uint8_t* data,
    size_t length,
    bool isNotify)
{
    LOG(
        LOG_CONTROL,
        String("Find My Sound: ")
        + (isNotify ? "notification" : "indication")
        + " from "
        + (
            characteristic != nullptr
                ? characteristic->getUUID().toString().c_str()
                : "unknown"
          )
        + " length="
        + String(length)
    );

    String hex;

    for (size_t i = 0; i < length; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X ", data[i]);
        hex += buf;
    }

    if (length > 0) {
        LOG(LOG_CONTROL, "Find My Sound: response " + hex);
    }

    // Recognizes this DULT response.
    if (length >= 2 &&
        data[0] == 0x02 &&
        data[1] == 0x03) {

        LOG(LOG_CONTROL, "Find My Sound: DULT Sound_Completed received");
    }
}

// ============================================================
// Enable tracker responses
//
// Prefers indications and falls back to notifications.
// ============================================================

bool enableTrackerResponses(
    NimBLERemoteCharacteristic* characteristic)
{
    if (characteristic == nullptr) {
        return false;
    }

    if (characteristic->canIndicate()) {

        LOG(LOG_CONTROL, "Find My Sound: enabling indications");

        if (!characteristic->subscribe(
                false,
                trackerNotifyCallback,
                true)) {

            LOG(LOG_CONTROL, "Find My Sound: failed to enable indications");

            return false;
        }

        return true;
    }

    if (characteristic->canNotify()) {

        LOG(LOG_CONTROL, "Find My Sound: enabling notifications");

        if (!characteristic->subscribe(
                true,
                trackerNotifyCallback,
                true)) {

            LOG(LOG_CONTROL, "Find My Sound: failed to enable notifications");

            return false;
        }

        return true;
    }

    LOG(LOG_CONTROL, "Find My Sound: characteristic has no NOTIFY/INDICATE");

    return false;
}

// ============================================================
// Identify tracker and characteristic
// ============================================================

TrackerType identifyTracker(
    NimBLEClient* client,
    NimBLERemoteCharacteristic*& soundCharacteristic)
{
    soundCharacteristic = nullptr;

    if (client == nullptr ||
        !client->isConnected()) {

        return TrackerType::Unknown;
    }

    bool hasServices = false;

    bool hasAirTagService = false;
    bool hasFMNAService   = false;
    bool hasDULTService   = false;

    bool hasAirTagSound = false;
    bool hasFMNASound   = false;
    bool hasDULTSound   = false;

    LOG(LOG_CONTROL, "Find My Sound: discovering services");

    const auto& services = client->getServices(true);

    for (NimBLERemoteService* service : services) {

        if (service == nullptr ||
            !client->isConnected()) {

            break;
        }

        hasServices = true;

        const NimBLEUUID serviceUuid =
            service->getUUID();

        LOG(LOG_CONTROL, "Find My Sound: service " + String(serviceUuid.toString().c_str()));

        if (serviceUuid.equals(AIRTAG_SERVICE_UUID)) {

            hasAirTagService = true;

            LOG(LOG_CONTROL, "Find My Sound: AirTag service detected");
        }

        if (serviceUuid.equals(FMNA_SERVICE_UUID)) {

            hasFMNAService = true;

            LOG(LOG_CONTROL, "Find My Sound: FMNA service detected");
        }

        if (serviceUuid.equals(DULT_SERVICE_UUID)) {

            hasDULTService = true;

            LOG(LOG_CONTROL, "Find My Sound: DULT service detected");
        }

        const auto& characteristics =
            service->getCharacteristics(true);

        for (NimBLERemoteCharacteristic* characteristic :
             characteristics) {

            if (characteristic == nullptr ||
                !client->isConnected()) {

                break;
            }

            const NimBLEUUID characteristicUuid =
                characteristic->getUUID();

            LOG(LOG_CONTROL, "Find My Sound: characteristic " + String(characteristicUuid.toString().c_str()));

            if (serviceUuid.equals(AIRTAG_SERVICE_UUID) &&
                characteristicUuid.equals(
                    AIRTAG_CHARACTERISTIC_UUID)) {

                hasAirTagSound = true;
                soundCharacteristic = characteristic;

                LOG(LOG_CONTROL, "Find My Sound: AirTag sound characteristic detected");
            }

            if (serviceUuid.equals(FMNA_SERVICE_UUID) &&
                characteristicUuid.equals(
                    FMNA_SOUND_CHARACTERISTIC_UUID)) {

                hasFMNASound = true;
                soundCharacteristic = characteristic;

                LOG(LOG_CONTROL, "Find My Sound: FMNA sound characteristic detected");
            }

            if (serviceUuid.equals(DULT_SERVICE_UUID) &&
                characteristicUuid.equals(
                    DULT_SOUND_CHARACTERISTIC_UUID)) {

                hasDULTSound = true;
                soundCharacteristic = characteristic;

                LOG(LOG_CONTROL, "Find My Sound: DULT sound characteristic detected");
            }
        }
    }

    if (hasAirTagService && hasAirTagSound) {

        return TrackerType::AirTag;
    }

    if (hasFMNAService && hasFMNASound) {

        return TrackerType::FMNA;
    }

    if (hasDULTService && hasDULTSound) {

        return TrackerType::DULT;
    }

    if (hasServices) {

        LOG(LOG_CONTROL, "Find My Sound: services found, but no supported sound service");

        return TrackerType::Unknown;
    }

    return TrackerType::Unknown;
}

// ============================================================
// Send AirTag command
// ============================================================

Result sendAirTagSound(
    NimBLEClient* client,
    NimBLERemoteCharacteristic* characteristic)
{
    if (client == nullptr ||
        !client->isConnected()) {

        return Result::ConnectionFailed;
    }

    if (characteristic == nullptr) {

        return Result::CharacteristicNotFound;
    }

    if (!characteristic->canWrite()) {

        LOG(LOG_CONTROL, "Find My Sound: AirTag characteristic does not advertise WRITE");

        return Result::CharacteristicNotWritable;
    }

    if (!client->isConnected()) {

        LOG(LOG_CONTROL, "Find My Sound: AirTag disconnected before command");

        return Result::ConnectionFailed;
    }

    LOG(LOG_CONTROL, "Find My Sound: sending AirTag unauthorized-sound command 0xAF");

    const bool written =
        characteristic->writeValue(
            &AIRTAG_BEEP_COMMAND,
            sizeof(AIRTAG_BEEP_COMMAND),
            true
        );

    if (!written) {

        LOG(LOG_CONTROL, "Find My Sound: AirTag sound command failed");

        return Result::WriteFailed;
    }

    LOG( LOG_CONTROL, "Find My Sound: AirTag command acknowledged");

    return Result::Success;
}

// ============================================================
// Send FMNA command
// ============================================================

Result sendFMNASound(
    NimBLEClient* client,
    NimBLERemoteCharacteristic* characteristic)
{
    if (client == nullptr ||
        !client->isConnected()) {

        return Result::ConnectionFailed;
    }

    if (characteristic == nullptr) {

        return Result::CharacteristicNotFound;
    }

    if (!characteristic->canWrite()) {

        LOG(LOG_CONTROL, "Find My Sound: FMNA characteristic does not advertise WRITE");

        return Result::CharacteristicNotWritable;
    }

    if (!enableTrackerResponses(characteristic)) {

        LOG(LOG_CONTROL, "Find My Sound: could not enable FMNA responses");

        return Result::WriteFailed;
    }

    LOG(LOG_CONTROL, "Find My Sound: sending FMNA Sound_Start 01 00 03");

    const bool written =
        characteristic->writeValue(
            FMNA_START_SOUND_COMMAND,
            sizeof(FMNA_START_SOUND_COMMAND),
            true
        );

    if (!written) {

        LOG(LOG_CONTROL, "Find My Sound: FMNA sound command failed");

        return Result::WriteFailed;
    }

    LOG(LOG_CONTROL, "Find My Sound: FMNA command acknowledged");

    const uint32_t timeoutAt = millis() + 2000;

    while (client->isConnected() &&
           static_cast<int32_t>(timeoutAt - millis()) > 0) {

        delay(10);
    }

    return Result::Success;
}

// ============================================================
// Send DULT command
// ============================================================

Result sendDULTSound(
    NimBLEClient* client,
    NimBLERemoteCharacteristic* characteristic)
{
    if (client == nullptr ||
        !client->isConnected()) {

        return Result::ConnectionFailed;
    }

    if (characteristic == nullptr) {

        return Result::CharacteristicNotFound;
    }

    if (!characteristic->canWrite()) {

        LOG(LOG_CONTROL, "Find My Sound: DULT characteristic does not advertise WRITE");

        return Result::CharacteristicNotWritable;
    }

    if (!enableTrackerResponses(characteristic)) {

        LOG(LOG_CONTROL, "Find My Sound: could not enable DULT responses");

        return Result::WriteFailed;
    }

    LOG(LOG_CONTROL, "Find My Sound: sending DULT Sound_Start 00 03");

    const bool written =
        characteristic->writeValue(
            DULT_START_SOUND_COMMAND,
            sizeof(DULT_START_SOUND_COMMAND),
            true
        );

    if (!written) {

        LOG(LOG_CONTROL, "Find My Sound: DULT sound command failed");

        return Result::WriteFailed;
    }

    LOG(LOG_CONTROL, "Find My Sound: DULT command acknowledged");

    const uint32_t timeoutAt = millis() + 3000;

    while (client->isConnected() &&
           static_cast<int32_t>(timeoutAt - millis()) > 0) {

        delay(10);
    }

    return Result::Success;
}

} // anonymous namespace


// ============================================================
// Public API
// ============================================================

Result play(const String& address)
{
    if (address.isEmpty()) {

        LOG(LOG_CONTROL, "Find My Sound: invalid address");

        return Result::InvalidAddress;
    }

    if (!NimBLEDevice::isInitialized()) {

        LOG(LOG_CONTROL, "Find My Sound: NimBLE is not initialized");

        return Result::BluetoothNotInitialized;
    }

    LOG(LOG_CONTROL, "Find My Sound: starting operation for " + address);

    /*
     * Find My trackers normally advertise using random BLE
     * addresses.
     */
    NimBLEAddress peerAddress(
        address.c_str(),
        BLE_ADDR_RANDOM
    );

    NimBLEClient* client =
        NimBLEDevice::createClient();

    if (client == nullptr) {

        LOG(LOG_CONTROL, "Find My Sound: unable to create BLE client");

        return Result::ClientCreationFailed;
    }

    Result result = Result::UnknownError;

    /*
     * Disables authentication requirements.
     *
     * We don't proactively request:
     *   - bonding
     *   - MITM
     *   - secure connections
     */
    NimBLEDevice::setSecurityAuth(
        false,
        false,
        false
    );

    /*
     * Use 15 second connection timeout.
     */
    client->setConnectTimeout(
        CONNECT_TIMEOUT_MS
    );

    LOG(LOG_CONTROL,"Find My Sound: connecting to " + address);

    /*
     * Connect() parameters:
     *
     *   true  = refresh services
     *   false = don't delete old services
     *   true  = async connection handling disabled / direct connect
     */
    if (!client->connect(
            peerAddress,
            true,
            false,
            true)) {

        LOG(LOG_CONTROL, "Find My Sound: connection failed; error=" + String(client->getLastError()));

        LOG(LOG_CONTROL, "Find My Sound: connected=" + String(client->isConnected() ? 1 : 0));

        result = Result::ConnectionFailed;

        NimBLEDevice::deleteClient(client);

        return result;
    }

    LOG(
        LOG_CONTROL,
        "Find My Sound: connected"
    );

    /*
     * Discover the actual Find My sound service and
     * characteristic instead of blindly assuming that
     * the device is an AirTag.
     */
    NimBLERemoteCharacteristic* soundCharacteristic =
        nullptr;

    TrackerType trackerType =
        identifyTracker(
            client,
            soundCharacteristic
        );

    LOG(LOG_CONTROL, "Find My Sound: tracker type = " + String(trackerTypeToString(trackerType)));

    if (trackerType == TrackerType::Unknown ||
        soundCharacteristic == nullptr) {

        result = Result::ServiceNotFound;

    } else {

        switch (trackerType) {

            case TrackerType::AirTag:

                result =
                    sendAirTagSound(
                        client,
                        soundCharacteristic
                    );

                break;

            case TrackerType::FMNA:

                result =
                    sendFMNASound(
                        client,
                        soundCharacteristic
                    );

                break;

            case TrackerType::DULT:

                result =
                    sendDULTSound(
                        client,
                        soundCharacteristic
                    );

                break;

            case TrackerType::Unknown:
            default:

                result = Result::NotSupported;

                break;
        }
    }

    /*
     * Disconnect before deleting the temporary client.
     */
    if (client->isConnected()) {

        LOG(LOG_CONTROL, "Find My Sound: disconnecting");

        client->disconnect();
    }

    NimBLEDevice::deleteClient(client);

    LOG(LOG_TARGET, String("     Find My Sound: finished with result = ") + resultToString(result));

    return result;
}


// ============================================================
// Result strings
// ============================================================

const char* resultToString(Result result)
{
    switch (result) {

        case Result::Success:
            return "Success";

        case Result::InvalidAddress:
            return "Invalid address";

        case Result::BluetoothNotInitialized:
            return "Bluetooth not initialized";

        case Result::ClientCreationFailed:
            return "BLE client creation failed";

        case Result::ConnectionFailed:
            return "Connection failed";

        case Result::ServiceNotFound:
            return "Find My sound service not found";

        case Result::CharacteristicNotFound:
            return "Sound characteristic not found";

        case Result::CharacteristicNotWritable:
            return "Sound characteristic is not writable";

        case Result::WriteFailed:
            return "Sound command failed";

        case Result::NotSupported:
            return "Tracker type not supported";

        case Result::UnknownError:
        default:
            return "Unknown error";
    }
}


// ============================================================
// Tracker type strings
// ============================================================

const char* trackerTypeToString(TrackerType type)
{
    switch (type) {

        case TrackerType::AirTag:
            return "AirTag";

        case TrackerType::FMNA:
            return "FMNA";

        case TrackerType::DULT:
            return "DULT";

        case TrackerType::Unknown:
        default:
            return "Unknown";
    }
}

} // namespace FindMySound
