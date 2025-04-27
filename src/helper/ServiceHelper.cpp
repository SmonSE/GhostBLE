#include "ServiceHelper.h"

String getServiceName(const String& uuid) {
    if (uuid == "1800") return "Generic Access Service";
    if (uuid == "1801") return "Generic Attribute Service";
    if (uuid == "180f") return "Battery Service";
    if (uuid == "180d") return "Heart Rate Service";
    if (uuid == "180a") return "Device Information Service";
    if (uuid == "1802") return "Immediate Alert";
    if (uuid == "1803") return "Link Loss";
    if (uuid == "1804") return "Tx Power";
    if (uuid == "1805") return "Current Time Service";
    if (uuid == "1812") return "Human Interface Device (HID)";
    if (uuid == "1809") return "Health Thermometer";
    if (uuid == "1811") return "Alert Notification Service";
    if (uuid == "1810") return "Blood Pressure";
    if (uuid == "1813") return "Glucose";
    if (uuid == "1823") return "Running Speed and Cadence";
    if (uuid == "1824") return "Cycling Speed and Cadence";
    if (uuid == "1806") return "Scan Parameters";
    if (uuid == "1815") return "Temperature Measurement";
    if (uuid == "1816") return "Temperature Type";
    if (uuid == "1825") return "Cycling Power";
    if (uuid == "1826") return "Cycling Torque Measurement";
    if (uuid == "1827") return "Cycling Torque Vector";
    if (uuid == "1832") return "Barometric Pressure";
    if (uuid == "1833") return "Air Quality";
    if (uuid == "1834") return "Oxygen Saturation";
    if (uuid == "1835") return "Pollen Data";
    if (uuid == "1836") return "Personal Activity Monitoring";
    if (uuid == "1837") return "Fitness Machine";
    if (uuid == "1838") return "Health and Fitness Measurement";
    return "Unknown Service";
  }
  