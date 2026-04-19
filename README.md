# GhostBLE

![](logoConverter/rawConverter/anyone_here.png)    ![](logoConverter/rawConverter/thugLife.png)    ![](logoConverter/rawConverter/jbl_headphone.png)   

**A BLE privacy scanner for M5Stack devices**

GhostBLE discovers nearby Bluetooth Low Energy devices, analyzes their privacy posture, and flags potential security concerns. Built for security researchers, tinkerers, and educators interested in BLE privacy, device fingerprinting, and wireless reconnaissance.

A friendly mascot named **NibBLEs** guides you through the scanning process on the built-in display.

> [!WARNING]
> GhostBLE is intended for legal and authorized security testing, education, and privacy research only. Use of this software for any malicious or unauthorized activities is strictly prohibited. The developers assume no liability for misuse. Use at your own risk.

---

## Quick Start

1. Flash the firmware (see [Building & Flashing](#building--flashing))
2. Insert a **microSD card** (required on Cardputer for logging and XP)
3. Power on — NibBLEs greets you on the display
4. **Long press BtnA** (1 second) to start scanning
5. Watch devices appear on the display and in the logs

---

## Supported Hardware

| Device | Platform | Display | Keyboard | SD Card |
|--------|----------|---------|----------|---------|
| **M5Stack Cardputer** | ESP32-S3 | 240x135 LCD | Yes | Yes |
| **M5StickC Plus 2** | ESP32 | 240x135 LCD | No (2 buttons) | No |
| **M5StickS3** | ESP32-S3 | 240x135 LCD | No (2 buttons) | No |

All devices support BLE scanning, GATT connections, GPS wardriving, WiFi dashboard, and PwnBeacon. The Cardputer adds keyboard controls, SD card logging, XP persistence, and screenshot capture.

---

## Features

### BLE Scanning & Analysis

- **Passive BLE scanning** — discovers nearby devices with signal strength (RSSI) and estimated distance
- **Device info extraction** — retrieves names, service UUIDs, and manufacturer-specific data
- **GATT connections** — connects to devices and reads standard BLE services (Device Info, Battery, Heart Rate, Temperature, Generic Access, Current Time, TX Power, Immediate Alert, Link Loss)

### Privacy Heuristics

- **Rotating MAC detection** — flags devices using Resolvable Private Addresses (RPA)
- **Cleartext detection** — flags devices leaking unencrypted identifiers
- **Exposure classification** — rates devices into tiers (None, Passive, Active, Consent) with a privacy score

### Security Analysis

- Detects **writable characteristics** (potential for unauthorized writes)
- Identifies **DFU** and **UART** services (expanded attack surface)
- Checks for **encryption** on sensitive services
- Builds **device fingerprints** from advertised UUIDs and GATT profiles

### Known Device Detection

- **Flipper Zero** — detected by known service UUIDs (black, white, transparent variants)
- **CatHack / Apple Juice** — BLE spam tool detection
- **Tesla** — detected via iBeacon UUID, GATT service, and name pattern matching
- **LightBlue** — app-based BLE testing tool
- **PwnBeacon / Pwnagotchi** — detects and reads PwnGrid beacons (identity, face, pwnd counters, messages)

### PwnBeacon

GhostBLE acts as both a PwnBeacon **client** (scanner) and **server** (advertiser), compatible with [PwnBook](https://github.com/pfefferle/PwnBook) and [Palnagotchi](https://github.com/pfefferle/palnagotchi):

- Advertises as a PwnBeacon with SHA-256 fingerprint, identity JSON, face, and device name
- Detects nearby PwnBeacon peers via advertisement service data
- Reads full peer info via GATT (identity, face, name, message)
- Signal/ping characteristic for peer interaction
- Pwnd counters update dynamically during scanning

### Manufacturer Identification

Decodes manufacturer data for Apple, Google, Samsung, Epson, and more. Also parses **iBeacon** advertisements (UUID, major, minor, TX power, distance).

### GPS & Wardriving

- **Dual GPS support** — Grove UART and LoRa cap GPS (Cardputer only for LoRa)
- **WiGLE CSV export** — log devices with location for mapping and analysis

### Logging

All findings are logged to multiple channels simultaneously:

| Channel | Details |
|---------|---------|
| **Serial** | 115200 baud, structured output |
| **SD card** | Per-category log files (Cardputer only) |
| **Web dashboard** | Real-time via WiFi AP and WebSocket |

Logs are organized into categories: Scan, GATT, Privacy, Security, Beacon, Control, GPS, System, Target, and Notify. Each device gets a **session ID** for cross-log correlation.

### XP System

GhostBLE gamifies the scanning process with experience points:

| Event | XP |
|-------|-----|
| Device discovered | +0.1 |
| GATT connection success | +0.5 |
| Characteristic subscription | +1.0 |
| PwnBeacon detected | +1.0 |
| UINT/FLOAT payload decoded | +1.0 |
| Notify data received | +1.5 |
| Manufacturer data decoded | +2.0 |
| Suspicious device found | +2.0 |
| Known characteristic decoded | +2.5 |
| iBeacon parsed | +3.0 |

XP is persisted to the SD card (Cardputer) and shown on the display with level progression.

### NibBLEs

NibBLEs is the on-screen mascot with context-sensitive expressions and speech bubbles:

- **Expressions** — happy, sad, angry, glasses (detective), thug life, sleeping, hearts, and more
- **Speech bubbles** — context-aware messages for idle, scan start, wardriving, suspicious finds, and level-ups
- **Screenshot capture** — press ENTER on Cardputer to save the current display to SD

---

## Controls

### Cardputer (Keyboard)

| Key | Action |
|-----|--------|
| **Long press BtnA** (1s) | Toggle BLE scanning on/off |
| **ENTER** | Capture screenshot to SD card |
| **FN** | Toggle WiFi AP and web server |
| **TAB** | Toggle wardriving mode |
| **DEL** | Switch GPS source (Grove / LoRa) |
| **H** | Help Control |
| **S** | SCAN Mode |
| **M** | Marker in logfile |

### M5StickC Plus 2 / M5StickS3 (Buttons)

| Button | Action |
|--------|--------|
| **BtnA short press** | Toggle WiFi AP and web server |
| **BtnA long press** (1s) | Toggle BLE scanning on/off |
| **BtnB short press** | Toggle wardriving mode |
| **BtnB long press** (1s) | Switch GPS source |

### Web Interface

1. Enable WiFi (press **FN** on Cardputer or **BtnA** on StickC/StickS3)
2. Connect to WiFi AP **`GhostBLE`** (password: **`ghostble123!`**)
3. Open **`192.168.4.1`** in a browser
4. View real-time device discovery logs via WebSocket

---

## Building & Flashing

### PlatformIO (Recommended)

```bash
# Cardputer
pio run -e ghostble -t upload

# M5StickC Plus 2
pio run -e ghostble-stickcplus2 -t upload

# M5StickS3
pio run -e ghostble-sticks3 -t upload

# Google Test
pio test -e native -v
```

### Arduino IDE

1. Install the **M5Stack board package** via Board Manager
2. Select the appropriate board for your device
3. Install the required libraries via Library Manager:
   - `M5Cardputer` / `M5StickCPlus2` / `M5Unified` — hardware abstraction (depends on device)
   - `NimBLE-Arduino` — BLE stack
   - `ESPAsyncWebServer`, `AsyncTCP` — web dashboard
   - `TinyGPSPlus` — GPS parsing
4. Open `GhostBLE.ino`, compile, and upload

---

## Sample Output

```
[#17] Advertised Services (1):
     - fe78 (Unknown Service)
[#17] 🔓 Connected and discovered attributes: be:e9:2f:33:47:f1
[#17] Reading Generic Access Service (0x1800)
   Device found: ENVY Photo 6200 series [be:e9:2f:33:47:f1]
   Generic Access Service found (0x1800)
     Read value of generic access info
     Device Name: ENVY Photo 6200 series
     Appearance: Unknown (0x0)
     PPCP - Min: 6, Max: 6, Latency: 0, Timeout: 2000
     Central Address Resolution: Not Supported
     Generic Attribute Service detected (0x1801)
     Service Changed: Supported (indicate)
     Unknown Service (0xfe78)
       Char 73fd8f50-626c-4f9b-a52e-b1d226efcf8d [RN] (len=4) = C0 A8 B2 35 (192.168.178.53)
       Char 262040ed-6f79-41bb-b657-bff4cb49195a [RN] (len=16) = 2A 02  ... (2A02:8071:2287:5B20:BEE9:2FFF:FE33:C7F1)
       Char 58633f16-5cad-46bd-978d-fa0ad01a45ea [R] (len=16) = 0C 76 32 66 4E A8 51 21 6D CA D9 B8 50 B3 E9 3D 
       Char 380c09f8-9665-417a-bb2b-06cb6a76e784 [R] (len=6) = 00 00 00 00 00 00 
       Char 8fe0b1c0-ea32-11e5-a4fc-0002a5d5c51b [RN] (len=58) = 01 20 E2 F8 8D C4 24 90 AE 79 A8 DF 86 8E 47 ...
       Char 17d096e0-ea1f-11e5-9dbc-0002a5d5c51b [RN] (len=51) = 00 00 ...
     Unknown Service (0xfe77)
       Char aec832f7-7dff-4d6e-9b65-5b5bcf753941 [R] (len=1) = 00 
       Char 8cec8341-c2b8-4744-b491-6826c665f187 [W]
       Char d87a143d-16f7-4c20-9b73-2e24a8dfbcac [R] (len=1) = 00 
       Char c4be737a-c1ed-44a4-b115-b28bddba8f45 [RNI] (len=1) = 00 
     Unknown Service (0x7365a0ae-e596-129d-d84a-88db1ffbcc04)
       Char 1c7cfacb-7818-c09c-9345-04602070e0cc [RW] (len=1) = 00 
       Char ddbb8ffd-d1b6-bbb5-0f47-87a23630038b [RNI] (len=1) = 00 
[#17] Descriptor [73fd8f50-626c-4f9b-a52e-b1d226efcf8d]: IPv4 Address
[#17] Descriptor [262040ed-6f79-41bb-b657-bff4cb49195a]: IPv6 Address
[#17] Descriptor [58633f16-5cad-46bd-978d-fa0ad01a45ea]: Device UUID
[#17] Descriptor [380c09f8-9665-417a-bb2b-06cb6a76e784]: P2P Device ID
[#17] Descriptor [8fe0b1c0-ea32-11e5-a4fc-0002a5d5c51b]: Wi-Fi Infrastructure
[#17] Descriptor [17d096e0-ea1f-11e5-9dbc-0002a5d5c51b]: Micro Ap Configuration
[#17] No Target detected in gatt with address: be:e9:2f:33:47:f1
[#17] Device Infos
   Adress:  be:e9:2f:33:47:f1
   Name:    ENVY Photo 6200 series
   Manuf.:  Sony Ericsson Mobile Communications
   Raw GATT:
     - ENVY Photo 6200 series
     - IPv4 Address
     - IPv6 Address
     - Device UUID
     - P2P Device ID
     - Wi-Fi Infrastructure
     - Micro Ap Configuration
   Distance: 19.95 m
   RSSI: -95
```

---

## Project Structure

```
GhostBLE/
├── src/
│
│   ├── app/                 # Application Layer (Use Cases)
│   │   ├── context/
│   │   ├── controller/
│   │   ├── device/
│   │   └── gamification/
│   │
│   ├── core/                # Domain Layer (Core Logic)
│   │   ├── models/
│   │   ├── parsing/
│   │   ├── detection/
│   │   ├── fingerprint/
│   │   ├── filtering/
│   │   ├── analysis/
│   │   ├── privacy/
│   │   └── gatt/
│   │
│   ├── infrastructure/      # Infrastructure Layer
│   │   ├── ble/
│   │   ├── logging/
│   │   ├── gps/
│   │   ├── storage/
│   │   └── platform/        # Hardware (M5, ESP32 etc.)
│   │
│   ├── ui/                  # Presentation Layer
│   │   ├── display/
│   │   ├── overlay/
│   │   ├── icons/
│   │   └── screens/
│   │
│   ├── config/              # Configuration
│   │   ├── app_config
│   │   ├── detection_config
│   │   ├── signal_config
│   │   └── ...
│   │
│   ├── assets/              # Assets / Resources
│   │   ├── images/
│   │   └── gatt/ (optional)
│   │
│   └── main.cpp             # Entry Point
│
├── tests/                   # Tests / Unit Tests
```

---

## Contributing

PRs and suggestions welcome! This is a learning-focused BLE project, and your ideas are appreciated.

## License

MIT License
