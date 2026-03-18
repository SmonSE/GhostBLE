#include "ServiceHelper.h"
#include "../config/config.h"

// Sorted lookup table for standard 16-bit BLE service UUIDs.
// Binary search is O(log n) with no heap allocation vs cascading String comparisons.
struct ServiceEntry {
    uint16_t uuid;
    const char* name;
};

static const ServiceEntry serviceTable[] = {
    { 0x1800, "Generic Access Service" },
    { 0x1801, "Generic Attribute Service" },
    { 0x1802, "Immediate Alert" },
    { 0x1803, "Link Loss" },
    { 0x1804, "Tx Power" },
    { 0x1805, "Current Time Service" },
    { 0x1806, "Scan Parameters" },
    { 0x1809, "Health Thermometer" },
    { 0x180a, "Device Information Service" },
    { 0x180d, "Heart Rate Service" },
    { 0x180f, "Battery Service" },
    { 0x1810, "Blood Pressure" },
    { 0x1811, "Alert Notification Service" },
    { 0x1812, "Human Interface Device (HID)" },
    { 0x1813, "Glucose" },
    { 0x1814, "Running Speed and Cadence Service (RSCS)" },
    { 0x1815, "Temperature Measurement" },
    { 0x1816, "Temperature Type" },
    { 0x1823, "Running Speed and Cadence" },
    { 0x1824, "Cycling Speed and Cadence" },
    { 0x1825, "Cycling Power" },
    { 0x1826, "Cycling Torque Measurement" },
    { 0x1827, "Cycling Torque Vector" },
    { 0x1832, "Barometric Pressure" },
    { 0x1833, "Air Quality" },
    { 0x1834, "Oxygen Saturation" },
    { 0x1835, "Pollen Data" },
    { 0x1836, "Personal Activity Monitoring" },
    { 0x1837, "Fitness Machine" },
    { 0x1838, "Health and Fitness Measurement" },
};

static const int serviceTableSize = sizeof(serviceTable) / sizeof(serviceTable[0]);

String getServiceName(const String& uuid) {
    // Try parsing as a 16-bit UUID for fast binary search
    if (uuid.length() <= 4) {
        uint16_t id = (uint16_t)strtoul(uuid.c_str(), nullptr, 16);
        if (id != 0) {
            int lo = 0, hi = serviceTableSize - 1;
            while (lo <= hi) {
                int mid = (lo + hi) / 2;
                if (serviceTable[mid].uuid == id) return serviceTable[mid].name;
                if (serviceTable[mid].uuid < id) lo = mid + 1;
                else hi = mid - 1;
            }
        }
    }

    // Fall through for 128-bit UUIDs (vendor-specific)
    if (uuid == PWNBEACON_SERVICE_UUID) return "PwnBeacon (PwnGrid/BLE)";
    if (uuid == TESLA_BLE_SERVICE_UUID) return "Tesla Vehicle (BLE Key)";
    return "Unknown Service";
}
