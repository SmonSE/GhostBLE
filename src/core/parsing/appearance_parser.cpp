#include "appearance_parser.h"

// ============================================================
//  Appearance decoder
//  Source: Bluetooth Assigned Numbers, Section 2.6
//  Version: 2026-04-30
//
//  Format: bits 15..6 = category, bits 5..0 = subcategory
//  subcategory 0x00 = "Generic <Category>"
// ============================================================

String getAppearanceName(uint16_t appearance) {

    switch (appearance) {

    case 0x0000: return "Unknown";

    // Phone
    case 0x0040: return "Phone";

    // Computer
    case 0x0080: return "Computer";
    case 0x0081: return "Desktop Workstation";
    case 0x0082: return "Server";
    case 0x0083: return "Laptop";
    case 0x0084: return "Handheld PC / PDA";
    case 0x0085: return "Palm-size PC / PDA";
    case 0x0086: return "Wearable Computer (watch size)";
    case 0x0087: return "Tablet";
    case 0x0088: return "Docking Station";
    case 0x0089: return "All-in-One";
    case 0x008A: return "Blade Server";
    case 0x008B: return "Convertible";
    case 0x008C: return "Detachable";
    case 0x008D: return "IoT Gateway";
    case 0x008E: return "Mini PC";
    case 0x008F: return "Stick PC";

    // Watch
    case 0x00C0: return "Watch";
    case 0x00C1: return "Sports Watch";
    case 0x00C2: return "Smartwatch";

    // Clock / Display / Remote / Eye-glasses / Tag / Keyring / Media / Barcode
    case 0x0100: return "Clock";
    case 0x0140: return "Display";
    case 0x0180: return "Remote Control";
    case 0x01C0: return "Eye-glasses";
    case 0x0200: return "Tag";
    case 0x0240: return "Keyring";
    case 0x0280: return "Media Player";
    case 0x02C0: return "Barcode Scanner";

    // Thermometer
    case 0x0300: return "Thermometer";
    case 0x0301: return "Ear Thermometer";

    // Heart Rate Sensor
    case 0x0340: return "Heart Rate Sensor";
    case 0x0341: return "Heart Rate Belt";

    // Blood Pressure
    case 0x0380: return "Blood Pressure Monitor";
    case 0x0381: return "Arm Blood Pressure Monitor";
    case 0x0382: return "Wrist Blood Pressure Monitor";

    // HID
    case 0x03C0: return "HID Device";
    case 0x03C1: return "Keyboard";
    case 0x03C2: return "Mouse";
    case 0x03C3: return "Joystick";
    case 0x03C4: return "Gamepad";
    case 0x03C5: return "Digitizer Tablet";
    case 0x03C6: return "Card Reader";
    case 0x03C7: return "Digital Pen";
    case 0x03C8: return "Barcode Scanner (HID)";
    case 0x03C9: return "Touchpad";
    case 0x03CA: return "Presentation Remote";

    // Glucose Meter
    case 0x0400: return "Glucose Meter";

    // Running / Walking Sensor
    case 0x0440: return "Running / Walking Sensor";
    case 0x0441: return "In-Shoe Running Sensor";
    case 0x0442: return "On-Shoe Running Sensor";
    case 0x0443: return "On-Hip Running Sensor";

    // Cycling
    case 0x0480: return "Cycling";
    case 0x0481: return "Cycling Computer";
    case 0x0482: return "Speed Sensor";
    case 0x0483: return "Cadence Sensor";
    case 0x0484: return "Power Sensor";
    case 0x0485: return "Speed and Cadence Sensor";

    // Control Device
    case 0x04C0: return "Control Device";
    case 0x04C1: return "Switch";
    case 0x04C2: return "Multi-switch";
    case 0x04C3: return "Button";
    case 0x04C4: return "Slider";
    case 0x04C5: return "Rotary Switch";
    case 0x04C6: return "Touch Panel";
    case 0x04C7: return "Single Switch";
    case 0x04C8: return "Double Switch";
    case 0x04C9: return "Triple Switch";
    case 0x04CA: return "Battery Switch";
    case 0x04CB: return "Energy Harvesting Switch";
    case 0x04CC: return "Push Button";
    case 0x04CD: return "Dial";

    // Network Device
    case 0x0500: return "Network Device";
    case 0x0501: return "Access Point";
    case 0x0502: return "Mesh Device";
    case 0x0503: return "Mesh Network Proxy";

    // Sensor
    case 0x0540: return "Sensor";
    case 0x0541: return "Motion Sensor";
    case 0x0542: return "Air Quality Sensor";
    case 0x0543: return "Temperature Sensor";
    case 0x0544: return "Humidity Sensor";
    case 0x0545: return "Leak Sensor";
    case 0x0546: return "Smoke Sensor";
    case 0x0547: return "Occupancy Sensor";
    case 0x0548: return "Contact Sensor";
    case 0x0549: return "Carbon Monoxide Sensor";
    case 0x054A: return "Carbon Dioxide Sensor";
    case 0x054B: return "Ambient Light Sensor";
    case 0x054C: return "Energy Sensor";
    case 0x054D: return "Color Light Sensor";
    case 0x054E: return "Rain Sensor";
    case 0x054F: return "Fire Sensor";
    case 0x0550: return "Wind Sensor";
    case 0x0551: return "Proximity Sensor";
    case 0x0552: return "Multi-Sensor";
    case 0x0553: return "Flush Mounted Sensor";
    case 0x0554: return "Ceiling Mounted Sensor";
    case 0x0555: return "Wall Mounted Sensor";
    case 0x0556: return "Multisensor";
    case 0x0557: return "Energy Meter";
    case 0x0558: return "Flame Detector";
    case 0x0559: return "Vehicle Tire Pressure Sensor";

    // Light Fixtures
    case 0x0580: return "Light Fixture";
    case 0x0581: return "Wall Light";
    case 0x0582: return "Ceiling Light";
    case 0x0583: return "Floor Light";
    case 0x0584: return "Cabinet Light";
    case 0x0585: return "Desk Light";
    case 0x0586: return "Troffer Light";
    case 0x0587: return "Pendant Light";
    case 0x0588: return "In-ground Light";
    case 0x0589: return "Flood Light";
    case 0x058A: return "Underwater Light";
    case 0x058B: return "Bollard with Light";
    case 0x058C: return "Pathway Light";
    case 0x058D: return "Garden Light";
    case 0x058E: return "Pole-top Light";
    case 0x058F: return "Spotlight";
    case 0x0590: return "Linear Light";
    case 0x0591: return "Street Light";
    case 0x0592: return "Shelves Light";
    case 0x0593: return "Bay Light";
    case 0x0594: return "Emergency Exit Light";
    case 0x0595: return "Light Controller";
    case 0x0596: return "Light Driver";
    case 0x0597: return "Bulb";
    case 0x0598: return "Low-bay Light";
    case 0x0599: return "High-bay Light";

    // Fan
    case 0x05C0: return "Fan";
    case 0x05C1: return "Ceiling Fan";
    case 0x05C2: return "Axial Fan";
    case 0x05C3: return "Exhaust Fan";
    case 0x05C4: return "Pedestal Fan";
    case 0x05C5: return "Desk Fan";
    case 0x05C6: return "Wall Fan";

    // HVAC
    case 0x0600: return "HVAC";
    case 0x0601: return "Thermostat";
    case 0x0602: return "Humidifier";
    case 0x0603: return "De-humidifier";
    case 0x0604: return "Heater";
    case 0x0605: return "Radiator";
    case 0x0606: return "Boiler";
    case 0x0607: return "Heat Pump";
    case 0x0608: return "Infrared Heater";
    case 0x0609: return "Radiant Panel Heater";
    case 0x060A: return "Fan Heater";
    case 0x060B: return "Air Curtain";

    // Air Conditioning / Humidifier / Heating
    case 0x0640: return "Air Conditioning";
    case 0x0680: return "Humidifier";
    case 0x06C0: return "Heating";
    case 0x06C1: return "Radiator (Heating)";
    case 0x06C2: return "Boiler (Heating)";
    case 0x06C3: return "Heat Pump (Heating)";
    case 0x06C4: return "Infrared Heater (Heating)";
    case 0x06C5: return "Radiant Panel Heater (Heating)";
    case 0x06C6: return "Fan Heater (Heating)";
    case 0x06C7: return "Air Curtain (Heating)";

    // Access Control
    case 0x0700: return "Access Control";
    case 0x0701: return "Access Door";
    case 0x0702: return "Garage Door";
    case 0x0703: return "Emergency Exit Door";
    case 0x0704: return "Access Lock";
    case 0x0705: return "Elevator";
    case 0x0706: return "Window";
    case 0x0707: return "Entrance Gate";
    case 0x0708: return "Door Lock";
    case 0x0709: return "Locker";

    // Motorized Device
    case 0x0740: return "Motorized Device";
    case 0x0741: return "Motorized Gate";
    case 0x0742: return "Awning";
    case 0x0743: return "Blinds or Shades";
    case 0x0744: return "Curtains";
    case 0x0745: return "Screen";

    // Power Device
    case 0x0780: return "Power Device";
    case 0x0781: return "Power Outlet";
    case 0x0782: return "Power Strip";
    case 0x0783: return "Plug";
    case 0x0784: return "Power Supply";
    case 0x0785: return "LED Driver";
    case 0x0786: return "Fluorescent Lamp Gear";
    case 0x0787: return "HID Lamp Gear";
    case 0x0788: return "Charge Case";
    case 0x0789: return "Power Bank";

    // Light Source
    case 0x07C0: return "Light Source";
    case 0x07C1: return "Incandescent Light Bulb";
    case 0x07C2: return "LED Lamp";
    case 0x07C3: return "HID Lamp";
    case 0x07C4: return "Fluorescent Lamp";
    case 0x07C5: return "LED Array";
    case 0x07C6: return "Multi-Color LED Array";
    case 0x07C7: return "Low Voltage Halogen";
    case 0x07C8: return "OLED";

    // Window Covering
    case 0x0800: return "Window Covering";
    case 0x0801: return "Window Shades";
    case 0x0802: return "Window Blinds";
    case 0x0803: return "Window Awning";
    case 0x0804: return "Window Curtain";
    case 0x0805: return "Exterior Shutter";
    case 0x0806: return "Exterior Screen";

    // Audio Sink
    case 0x0840: return "Audio Sink";
    case 0x0841: return "Standalone Speaker";
    case 0x0842: return "Soundbar";
    case 0x0843: return "Bookshelf Speaker";
    case 0x0844: return "Standmounted Speaker";
    case 0x0845: return "Speakerphone";

    // Audio Source
    case 0x0880: return "Audio Source";
    case 0x0881: return "Microphone";
    case 0x0882: return "Alarm";
    case 0x0883: return "Bell";
    case 0x0884: return "Horn";
    case 0x0885: return "Broadcasting Device";
    case 0x0886: return "Service Desk";
    case 0x0887: return "Kiosk";
    case 0x0888: return "Broadcasting Room";
    case 0x0889: return "Auditorium";

    // Motorized Vehicle
    case 0x08C0: return "Motorized Vehicle";
    case 0x08C1: return "Car";
    case 0x08C2: return "Large Goods Vehicle";
    case 0x08C3: return "2-Wheeled Vehicle";
    case 0x08C4: return "Motorbike";
    case 0x08C5: return "Scooter";
    case 0x08C6: return "Moped";
    case 0x08C7: return "3-Wheeled Vehicle";
    case 0x08C8: return "Light Vehicle";
    case 0x08C9: return "Quad Bike";
    case 0x08CA: return "Minibus";
    case 0x08CB: return "Bus";
    case 0x08CC: return "Trolley";
    case 0x08CD: return "Agricultural Vehicle";
    case 0x08CE: return "Camper / Caravan";
    case 0x08CF: return "Recreational Vehicle / Motor Home";

    // Domestic Appliance
    case 0x0900: return "Domestic Appliance";
    case 0x0901: return "Refrigerator";
    case 0x0902: return "Freezer";
    case 0x0903: return "Oven";
    case 0x0904: return "Microwave";
    case 0x0905: return "Toaster";
    case 0x0906: return "Washing Machine";
    case 0x0907: return "Dryer";
    case 0x0908: return "Coffee Maker";
    case 0x0909: return "Clothes Iron";
    case 0x090A: return "Curling Iron";
    case 0x090B: return "Hair Dryer";
    case 0x090C: return "Vacuum Cleaner";
    case 0x090D: return "Robotic Vacuum Cleaner";
    case 0x090E: return "Rice Cooker";
    case 0x090F: return "Clothes Steamer";

    // Wearable Audio
    case 0x0940: return "Wearable Audio Device";
    case 0x0941: return "Earbud";
    case 0x0942: return "Headset";
    case 0x0943: return "Headphones";
    case 0x0944: return "Neck Band";
    case 0x0945: return "Left Earbud";
    case 0x0946: return "Right Earbud";

    // Aircraft
    case 0x0980: return "Aircraft";
    case 0x0981: return "Light Aircraft";
    case 0x0982: return "Microlight";
    case 0x0983: return "Paraglider";
    case 0x0984: return "Large Passenger Aircraft";

    // AV Equipment
    case 0x09C0: return "AV Equipment";
    case 0x09C1: return "Amplifier";
    case 0x09C2: return "Receiver";
    case 0x09C3: return "Radio";
    case 0x09C4: return "Tuner";
    case 0x09C5: return "Turntable";
    case 0x09C6: return "CD Player";
    case 0x09C7: return "DVD Player";
    case 0x09C8: return "Bluray Player";
    case 0x09C9: return "Optical Disc Player";
    case 0x09CA: return "Set-Top Box";

    // Display Equipment
    case 0x0A00: return "Display Equipment";
    case 0x0A01: return "Television";
    case 0x0A02: return "Monitor";
    case 0x0A03: return "Projector";

    // Hearing Aid
    case 0x0A40: return "Hearing Aid";
    case 0x0A41: return "In-ear Hearing Aid";
    case 0x0A42: return "Behind-ear Hearing Aid";
    case 0x0A43: return "Cochlear Implant";

    // Gaming
    case 0x0A80: return "Gaming Device";
    case 0x0A81: return "Home Video Game Console";
    case 0x0A82: return "Portable Handheld Console";

    // Signage
    case 0x0AC0: return "Signage";
    case 0x0AC1: return "Digital Signage";
    case 0x0AC2: return "Electronic Label";

    // Pulse Oximeter
    case 0x0C40: return "Pulse Oximeter";
    case 0x0C41: return "Fingertip Pulse Oximeter";
    case 0x0C42: return "Wrist Pulse Oximeter";

    // Weight Scale
    case 0x0C80: return "Weight Scale";

    // Personal Mobility Device
    case 0x0CC0: return "Personal Mobility Device";
    case 0x0CC1: return "Powered Wheelchair";
    case 0x0CC2: return "Mobility Scooter";

    // Continuous Glucose Monitor
    case 0x0D00: return "Continuous Glucose Monitor";

    // Insulin Pump
    case 0x0D40: return "Insulin Pump";
    case 0x0D41: return "Insulin Pump (durable)";
    case 0x0D44: return "Insulin Pump (patch)";
    case 0x0D48: return "Insulin Pen";

    // Medication Delivery / Spirometer
    case 0x0D80: return "Medication Delivery";
    case 0x0DC0: return "Spirometer";
    case 0x0DC1: return "Handheld Spirometer";

    // Outdoor Sports Activity
    case 0x1440: return "Outdoor Sports Activity";
    case 0x1441: return "Location Display";
    case 0x1442: return "Location and Navigation Display";
    case 0x1443: return "Location Pod";
    case 0x1444: return "Location and Navigation Pod";

    // Industrial Measurement Device
    case 0x1480: return "Industrial Measurement Device";
    case 0x1481: return "Torque Testing Device";
    case 0x1482: return "Caliper";
    case 0x1483: return "Dial Indicator";
    case 0x1484: return "Micrometer";
    case 0x1485: return "Height Gauge";
    case 0x1486: return "Force Gauge";

    // Industrial Tools
    case 0x14C0: return "Industrial Tools";
    case 0x14C1: return "Machine Tool Holder";
    case 0x14C2: return "Clamping Device";
    case 0x14C3: return "Clamping Jaws / Jaw Chuck";
    case 0x14C4: return "Clamping (Collet) Chuck";
    case 0x14C5: return "Clamping Mandrel";
    case 0x14C6: return "Vise";
    case 0x14C7: return "Zero-Point Clamping System";
    case 0x14C8: return "Torque Wrench";
    case 0x14C9: return "Torque Screwdriver";

    // Cookware Device
    case 0x1500: return "Cookware Device";
    case 0x1501: return "Pot / Jug";
    case 0x1502: return "Pressure Cooker";
    case 0x1503: return "Slow Cooker";
    case 0x1504: return "Steam Cooker";
    case 0x1505: return "Saucepan";
    case 0x1506: return "Frying Pan";
    case 0x1507: return "Casserole";
    case 0x1508: return "Dutch Oven";
    case 0x1509: return "Grill Pan / Raclette Grill";
    case 0x150A: return "Braising Pan";
    case 0x150B: return "Wok";
    case 0x150C: return "Paella Pan";
    case 0x150D: return "Crepe Pan";
    case 0x150E: return "Tagine";
    case 0x150F: return "Fondue";
    case 0x1510: return "Lid";
    case 0x1511: return "Wired Probe";
    case 0x1512: return "Wireless Probe";
    case 0x1513: return "Baking Molds";
    case 0x1514: return "Baking Tray";

    default:
        // Return category name as fallback for unknown subcategories
        switch (appearance >> 6) {
            case 0x001: return "Phone";
            case 0x002: return "Computer";
            case 0x003: return "Watch";
            case 0x004: return "Clock";
            case 0x005: return "Display";
            case 0x006: return "Remote Control";
            case 0x007: return "Eye-glasses";
            case 0x008: return "Tag";
            case 0x009: return "Keyring";
            case 0x00A: return "Media Player";
            case 0x00B: return "Barcode Scanner";
            case 0x00C: return "Thermometer";
            case 0x00D: return "Heart Rate Sensor";
            case 0x00E: return "Blood Pressure";
            case 0x00F: return "HID Device";
            case 0x010: return "Glucose Meter";
            case 0x011: return "Running / Walking Sensor";
            case 0x012: return "Cycling";
            case 0x013: return "Control Device";
            case 0x014: return "Network Device";
            case 0x015: return "Sensor";
            case 0x016: return "Light Fixture";
            case 0x017: return "Fan";
            case 0x018: return "HVAC";
            case 0x019: return "Air Conditioning";
            case 0x01A: return "Humidifier";
            case 0x01B: return "Heating";
            case 0x01C: return "Access Control";
            case 0x01D: return "Motorized Device";
            case 0x01E: return "Power Device";
            case 0x01F: return "Light Source";
            case 0x020: return "Window Covering";
            case 0x021: return "Audio Sink";
            case 0x022: return "Audio Source";
            case 0x023: return "Motorized Vehicle";
            case 0x024: return "Domestic Appliance";
            case 0x025: return "Wearable Audio";
            case 0x026: return "Aircraft";
            case 0x027: return "AV Equipment";
            case 0x028: return "Display Equipment";
            case 0x029: return "Hearing Aid";
            case 0x02A: return "Gaming";
            case 0x02B: return "Signage";
            case 0x031: return "Pulse Oximeter";
            case 0x032: return "Weight Scale";
            case 0x033: return "Personal Mobility Device";
            case 0x034: return "Continuous Glucose Monitor";
            case 0x035: return "Insulin Pump";
            case 0x036: return "Medication Delivery";
            case 0x037: return "Spirometer";
            case 0x051: return "Outdoor Sports Activity";
            case 0x052: return "Industrial Measurement Device";
            case 0x053: return "Industrial Tools";
            case 0x054: return "Cookware Device";
            default:    return "Unknown (0x" + String(appearance, HEX) + ")";
        }
    }
}