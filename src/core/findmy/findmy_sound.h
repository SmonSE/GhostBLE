#pragma once

#include <Arduino.h>

namespace FindMySound {

enum class Result {
    Success,

    InvalidAddress,
    BluetoothNotInitialized,
    ClientCreationFailed,

    ConnectionFailed,
    ServiceNotFound,
    CharacteristicNotFound,
    CharacteristicNotWritable,

    WriteFailed,

    NotSupported,
    UnknownError
};

enum class TrackerType {
    Unknown,
    AirTag,
    FMNA,
    DULT
};

/**
 * Connect to a detected Find My tracker and request its
 * separated-device sound.
 *
 *   AirTag:
 *     Service        7dfc9000-7d1c-4951-86aa-8d9728f8d66c
 *     Characteristic 7dfc9001-7d1c-4951-86aa-8d9728f8d66c
 *     Command        AF
 *
 *   FMNA:
 *     Service        0000fd44-0000-1000-8000-00805f9b34fb
 *     Characteristic 4f860003-943b-49ef-bed4-2f730304427a
 *     Start command  01 00 03
 *
 *   DULT:
 *     Service        15190001-12f4-c226-88ed-2ac5579f2a85
 *     Characteristic 8e0c0001-1d68-fb92-bf61-48377421680e
 *     Start command  00 03
 *
 * @param address BLE address stored in SusLog.
 *
 * @return Result describing the operation.
 */
Result play(const String& address);

/**
 * Convert a result to a human-readable string.
 */
const char* resultToString(Result result);

/**
 * Convert a tracker type to a human-readable string.
 */
const char* trackerTypeToString(TrackerType type);

} // namespace FindMySound
