#include "AppearanceHelper.h"

String getAppearanceName(uint16_t appearance) {
    // BLE Assigned Numbers — Appearance Values (category = bits 15..6)
    uint16_t category = appearance >> 6;

    switch (category) {
        case 0:   return "Unknown";
        case 1:   return "Phone";
        case 2:   return "Computer";
        case 3:   return "Watch";
        case 4:   return "Clock";
        case 5:   return "Display";
        case 6:   return "Remote Control";
        case 7:   return "Eye Glasses";
        case 8:   return "Tag";
        case 9:   return "Keyring";
        case 10:  return "Media Player";
        case 11:  return "Barcode Scanner";
        case 12:  return "Thermometer";
        case 13:  return "Heart Rate Sensor";
        case 14:  return "Blood Pressure";
        case 15:  return "HID Device";
        case 16:  return "Glucose Meter";
        case 17:  return "Running/Walking Sensor";
        case 18:  return "Cycling Sensor";
        case 49:  return "Pulse Oximeter";
        case 50:  return "Weight Scale";
        case 51:  return "Personal Mobility";
        case 52:  return "Continuous Glucose Monitor";
        case 53:  return "Insulin Pump";
        case 54:  return "Medication Delivery";
        case 81:  return "Outdoor Sports Activity";
        default:  break;
    }

    // Sub-type matching for HID devices (category 15)
    switch (appearance) {
        case 961:  return "Keyboard";
        case 962:  return "Mouse";
        case 963:  return "Joystick";
        case 964:  return "Gamepad";
        case 965:  return "Digitizer Tablet";
        case 966:  return "Card Reader";
        case 967:  return "Digital Pen";
        case 968:  return "Barcode Scanner (HID)";
        default:   break;
    }

    return "Unknown (0x" + String(appearance, HEX) + ")";
}
