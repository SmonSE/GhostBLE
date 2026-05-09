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
| **M5Stack Cardputer** | ESP32-S3 | 240×135 LCD | Yes | Yes |
| **M5StickS3** | ESP32-S3 | 240×135 LCD | No (2 buttons) | No |

All devices support BLE scanning, GATT connections, GPS wardriving, WiFi dashboard, and PwnBeacon. The Cardputer adds keyboard controls, SD card logging, XP persistence, and screenshot capture.

---

## Features

### BLE Scanning & Analysis

- **Passive BLE scanning** — discovers nearby devices with signal strength (RSSI) and estimated distance
- **Device info extraction** — retrieves names, service UUIDs, manufacturer data, appearance and TX power
- **GATT connections** — connects to devices and reads standard BLE services (Device Info, Battery, Heart Rate, Temperature, Generic Access, Current Time, TX Power, Immediate Alert, Link Loss)
- **Notification capture** — subscribes to notifiable characteristics and decodes live sensor data (Heart Rate, SpO2, Temperature, CSC, RSC, Glucose, Battery)
- **Advertisement interval fingerprinting** — measures time between advertisements for passive device-class identification

### Beacon Detection

- **iBeacon** — parses UUID, major, minor, TX power and estimates distance
- **Eddystone** — decodes all four frame types:
  - **UID** — 16-byte namespace + instance identifier
  - **URL** — leaks raw URLs (internal hostnames, asset tracker URLs)
  - **TLM** — telemetry data (battery voltage, temperature, uptime)
  - **EID** — ephemeral rotating ID (privacy-aware device)
- **PwnBeacon / Pwnagotchi** — detects and reads PwnGrid beacons (identity, face, pwnd counters)

### Privacy Heuristics

- **Rotating MAC detection** — flags devices using Resolvable Private Addresses (RPA)
- **Cleartext detection** — flags devices leaking unencrypted identifiers
- **Exposure classification** — rates devices into tiers (None, Passive, Active, Consent) with a privacy score
- **Live sensor data streaming** — flags devices broadcasting sensor data without authentication

### Security Analysis

- Detects **writable characteristics** without authentication (potential attack surface)
- Identifies **DFU** and **UART** services (expanded attack surface)
- Checks for **encryption** on sensitive services
- Builds **device fingerprints** from advertised UUIDs and GATT profiles
- **Pairing / bonding state detection** — distinguishes open from bonded devices via ATT error codes

### Parsers

- **Manufacturer parser** — 60+ company IDs from Bluetooth Assigned Numbers Section 7
- **Member Service parser** — 100+ member service UUIDs (Section 3.11): Apple, Google, Samsung, Tesla, Xiaomi, Huawei, Garmin, Polar, Bose, Sennheiser, Medtronic, Abbott, Dexcom and more
- **Appearance parser** — full subcategory decoding (190+ entries from Section 2.6): watches, medical devices, domestic appliances, vehicles, industrial tools, cookware and more
- **SDO Service parser** — Section 3.10 special services: Matter (0xFFF6), Zigbee Direct (0xFFF7), ASTM Drone Remote ID (0xFFFA), Thread (0xFFFB), AirFuel (0xFFFC), FIDO U2F (0xFFFD)

### Known Device Detection

- **Flipper Zero** — detected by known service UUIDs
- **CatHack / Apple Juice** — BLE spam tool detection
- **Tesla** — detected via iBeacon UUID, GATT service, and name pattern matching
- **LightBlue** — app-based BLE testing tool
- **Drones** — ASTM Remote ID detection via SDO service UUID 0xFFFA
- **PwnBeacon / Pwnagotchi** — detects and reads PwnGrid beacons (identity, face, pwnd counters, messages)

### Drone Remote ID (ASTM F3411-22a)

When a drone broadcasting Remote ID is detected via BLE service `0xFFFA`:

```
[#3] SDO Match: ASTM Remote ID (ASTM Remote ID)
[#3] ASTM F3411 Remote ID (v2)
   Type:        Helicopter / Multirotor
   Serial:      1ZNDBK20D00089
   Operator ID: DEU-HH-123456
   Status:      Airborne
   Drone GPS:   48.123456, 8.654321
   Alt (geo):   87.5 m
   Alt (baro):  85.0 m
   Height:      42.0 m (above takeoff)
   Speed H:     3.5 m/s
   Speed V:     0.0 m/s
   Heading:     270 deg
   H-Acc:       <10 m
   Pilot GPS:   48.120000, 8.650000    ← pilot standing here
   Pilot Alt:   382.5 m
   EU Class:    C1
   Description: Delivery drone — authorized flight
```

> Remote ID is mandatory for drones >250g in the EU (2019/947) and US (FAA Part 89)
> since 2023. GhostBLE decodes all six ASTM F3411-22a message types: Basic ID,
> Location, Authentication, Self-ID, System (operator GPS), and Operator ID.

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
- **GeoJSON export** — RSSI heat map for direct import into QGIS or Google Maps

### Web Dashboard

Real-time BLE device discovery via WiFi Access Point and WebSocket:

- **Live device cards** — RSSI signal strength with color coding (green/amber/red), device type tags (CONN, iBeacon, PWN, NOTIFY, SUS)
- **Trace log** — color-coded by category (scan, gatt, security, beacon, notify, privacy)
- **Stats bar** — live counters for Spotted / Suspicious / Beacons / PwnBeacons
- **Device filter** — filter cards by name or MAC address
- **Log replay** — load and replay a recorded `sniffed.log` from SD card into the UI
- **Settings panel** — configure device name, face, WiFi SSID and password
- **Auto-reconnect** — WebSocket reconnects automatically after BLE scan interruptions
- **Display sleep** — display turns off automatically when a web client connects to save resources

### Logging

All findings are logged to multiple channels simultaneously:

| Channel | Details |
|---------|---------|
| **Serial** | 115200 baud, structured output |
| **SD card** | Per-category log files (Cardputer only) |
| **Web dashboard** | Real-time via WiFi AP and WebSocket |

Log categories: Scan, GATT, Privacy, Security, Beacon, Control, GPS, System, Target, Notify. Each device gets a **session ID** for cross-log correlation.

### XP System

GhostBLE gamifies the scanning process with experience points:

| Event | XP |
|-------|-----|
| Device discovered | +0.1 |
| GATT connection success | +0.5 |
| Characteristic subscription | +1.0 |
| PwnBeacon detected | +1.0 |
| Notify data (unknown char) | +1.5 |
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
- **Display sleep** — NibBLEs pauses animations when a web client is connected

---

## Controls

### Cardputer (Keyboard)

| Key | Action |
|-----|--------|
| **Long press BtnA** (1s) | Toggle BLE scanning on/off |
| **ENTER** | Capture screenshot to SD card |
| **FN** | Toggle WiFi AP and web dashboard |
| **TAB** | Toggle wardriving mode |
| **DEL** | Switch GPS source (Grove / LoRa) |
| **H** | Show help overlay |
| **S** | Scan mode |
| **M** | Place marker in log file |

### M5StickS3 (Buttons)

| Button | Action |
|--------|--------|
| **BtnA short press** | Toggle WiFi AP and web dashboard |
| **BtnA long press** (1s) | Toggle BLE scanning on/off |
| **BtnB short press** | Toggle wardriving mode |
| **BtnB long press** (1s) | Switch GPS source |

### Web Dashboard

1. Enable WiFi (**FN** on Cardputer or **BtnA** on StickS3)
2. Connect to WiFi AP **`GhostBLE`** (password: **`ghostble123!`**)
3. Open **`192.168.4.1`** in a browser
4. Use the **▶ REPLAY LOG** button in Settings to replay a recorded session from SD card

---

## Architecture

GhostBLE uses a layered architecture with namespaced context objects replacing global variables:

```
src/app/context/
├── scan_context.*      # BLE scan state, device dedup, counters (thread-safe atomics)
├── ui_context.*        # NibBLEs animations, display state, FreeRTOS task handles
├── network_context.*   # WiFi/WebSocket, wardriving, GPS, WiGLE logger
└── device_context.*    # DeviceConfig, XPManager, beacon counters
```

Threading model: BLE scanning runs on Core 1 via FreeRTOS. Variables shared between cores use `std::atomic<>`. UI and network operations run on Core 0 (Arduino loop).

---

## Building & Flashing

### PlatformIO (Recommended)

```bash
# Cardputer
pio run -e ghostble -t upload

# M5StickS3
pio run -e ghostble-sticks3 -t upload

# Run unit tests (native / Google Test)
pio test -e native -v
```

### Arduino IDE

1. Install the **M5Stack board package** via Board Manager
2. Select the appropriate board for your device
3. Install required libraries via Library Manager:
   - `M5Cardputer` / `M5Unified` — hardware abstraction
   - `NimBLE-Arduino` — BLE stack
   - `ESPAsyncWebServer`, `AsyncTCP` — web dashboard
   - `TinyGPSPlus` — GPS parsing
4. Open `GhostBLE.ino`, compile, and upload

---

## Sample Output

```
[#17] Advertised Services (1):
     - fe78 (Member Service — Woan Technology)
[#17] Connected and discovered attributes: be:e9:2f:33:47:f1
[#17] Reading Generic Access Service (0x1800)
   Device Name: ENVY Photo 6200 series
   Appearance: Unknown (0x0)
[#17] Device info
   Address:  be:e9:2f:33:47:f1
   Name:     ENVY Photo 6200 series
   Manuf.:   HP, Inc.
   Raw GATT:
     - IPv4 Address = (192.168.178.53)
     - IPv6 Address = (2A02:8071:2287:5B20:BEE9:2FFF:FE33:C7F1)
     - Device UUID
   Distance: ~19.95 m
   RSSI:     -95 dBm
[#17] Exposure summary
   Device type:       Printer / IoT Device
   Identity exposure: HIGH — IP address leaked via GATT
   Tracking risk:     HIGH — static MAC + identity data
   Privacy level:     VERY POOR
```

---

## Project Structure

```
GhostBLE/
├── src/
│   ├── app/
│   │   ├── context/         # Scan, UI, Network, Device context
│   │   ├── gamification/    # XP system
│   │   └── interaction/
│   ├── assets/              # NibBLEs sprite assets
│   ├── config/              # App, detection, signal config
│   ├── core/
│   │   ├── analyzer/        # Exposure, security, privacy analyzers
│   │   ├── detection/       # Known device detection
│   │   ├── fingerprint/     # Device fingerprinting
│   │   ├── models/          # DeviceInfo, ExposureResult
│   │   └── parsing/         # Appearance, service, SDO, Eddystone parsers
│   ├── infrastructure/
│   │   ├── ble/             # Scanner, GATT handlers, Eddystone
│   │   ├── gps/             # GPS manager
│   │   ├── logging/         # Multi-target logger
│   │   ├── platform/        # Hardware abstraction
│   │   └── wardriving/      # WiGLE logger
│   ├── ui/
│   │   ├── expression/      # NibBLEs animations
│   │   ├── icons/           # Status icons
│   │   └── overlay/         # Draw helpers
│   └── web/                 # Web dashboard, WebSender, LogReplayer
├── test/
│   ├── mocks/               # Arduino stubs for native builds
│   └── test_exposure/       # Google Test suite for exposure analyzer
└── platformio.ini
```

---

## Contributing

PRs and suggestions welcome. This is a learning-focused BLE privacy project — ideas, bug reports, and new device signatures are all appreciated.

## License

MIT License
