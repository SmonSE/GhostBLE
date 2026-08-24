#include "service_parser.h"
#include "config/detection_config.h"
#include "core/parsing/manufacturer_parser.h"

// Sorted lookup table for standard 16-bit BLE service UUIDs.
// Binary search is O(log n) with no heap allocation vs cascading String comparisons.
struct ServiceEntry {
    uint16_t uuid;
    const char* name;
};

static const ServiceEntry serviceTable[] = {

    // ===== Generic / GAP =====
    { 0x1800, "Generic Access Service" },
    { 0x1801, "Generic Attribute Service" },

    // ===== Alert / Device =====
    { 0x1802, "Immediate Alert Service" },
    { 0x1803, "Link Loss Service" },
    { 0x1804, "Tx Power Service" },
    { 0x1805, "Current Time Service" },
    { 0x1806, "Reference Time Update Service" },
    { 0x1807, "Next DST Change Service" },
    { 0x1808, "Glucose Service" },
    { 0x1809, "Health Thermometer Service" },
    { 0x180A, "Device Information Service" },
    { 0x180D, "Heart Rate Service" },
    { 0x180F, "Battery Service" },

    // ===== HID / Health =====
    { 0x1810, "Blood Pressure Service" },
    { 0x1811, "Alert Notification Service" },
    { 0x1812, "Human Interface Device Service" },
    { 0x1813, "Scan Parameters Service" },
    { 0x1814, "Running Speed and Cadence Service" },
    { 0x1815, "Automation IO Service" },
    { 0x1816, "Cycling Speed and Cadence Service" },
    { 0x1818, "Cycling Power Service" },
    { 0x1819, "Location and Navigation Service" },
    { 0x181A, "Environmental Sensing Service" },
    { 0x181B, "Body Composition Service" },
    { 0x181C, "User Data Service" },
    { 0x181D, "Weight Scale Service" },
    { 0x181E, "Bond Management Service" },
    { 0x181F, "Continuous Glucose Monitoring Service" },

    // ===== More modern BLE =====
    { 0x1820, "Internet Protocol Support Service" },
    { 0x1821, "Indoor Positioning Service" },
    { 0x1822, "Pulse Oximeter Service" },
    { 0x1823, "HTTP Proxy Service" },
    { 0x1824, "Transport Discovery Service" },
    { 0x1825, "Object Transfer Service" },
    { 0x1826, "Fitness Machine Service" },
    { 0x1827, "Mesh Provisioning Service" },
    { 0x1828, "Mesh Proxy Service" },
    { 0x1829, "Reconnection Configuration Service" },
    { 0x1830, "Insulin Delivery Service" },
    { 0x1831, "Binary Sensor Service" },
    { 0x1832, "Emergency Configuration Service" },
    { 0x1833, "Physical Activity Monitor Service" },
    { 0x1834, "Elapsed Time Service" },
    { 0x1835, "Generic Media Control Service" },
    { 0x1836, "Generic Telephone Bearer Service" },
    { 0x1837, "Generic Media Control Service" },
    { 0x1838, "Constant Tone Extension Service" },
};

static const int serviceTableSize = sizeof(serviceTable) / sizeof(serviceTable[0]);

String getServiceName(const String& uuid) {

    String normalized = uuid;
    normalized.trim();
    normalized.toLowerCase();

    if (normalized.startsWith("0x")) {
        normalized = normalized.substring(2);
    }

    // Try parsing as a 16-bit UUID
    if (normalized.length() == 4) {

        uint16_t id = (uint16_t)strtoul(normalized.c_str(), nullptr, 16);

        int lo = 0;
        int hi = serviceTableSize - 1;

        while (lo <= hi) {
            int mid = (lo + hi) / 2;

            if (serviceTable[mid].uuid == id) {
                return serviceTable[mid].name;
            }

            if (serviceTable[mid].uuid < id) {
                lo = mid + 1;
            } else {
                hi = mid - 1;
            }
        }

        // Bluetooth SIG Member Services
        String owner = getMemberServiceOwner(id);

        if (!owner.isEmpty()) {
            return "Member Service (" + owner + ")";
        }
    }

    // 128-bit vendor-specific UUIDs
    if (normalized.equalsIgnoreCase(PWNBEACON_SERVICE_UUID)) {
        return "PwnBeacon (PwnGrid/BLE)";
    }

    if (normalized.equalsIgnoreCase(TESLA_BLE_SERVICE_UUID)) {
        return "Tesla Vehicle (BLE Key)";
    }

    return "Unknown Service";
}
