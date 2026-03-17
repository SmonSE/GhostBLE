# GhostBLE

**A BLE privacy scanner for the M5Stack Cardputer**

GhostBLE discovers nearby Bluetooth Low Energy devices, analyzes their privacy posture, and flags potential security concerns. Built for security researchers, tinkerers, and educators interested in BLE privacy, device fingerprinting, and wireless reconnaissance.

A friendly mascot named **Nibbles** guides you through the scanning process on the built-in display.

> **Disclaimer:** GhostBLE is intended for legal and authorized security testing, education, and privacy research only. Use of this software for any malicious or unauthorized activities is strictly prohibited. The developers assume no liability for misuse. Use at your own risk.

---

## Quick Start

1. Insert a **microSD card** (required — the device halts without one)
2. Flash the firmware (see [Building & Flashing](#building--flashing))
3. Power on — Nibbles greets you on the display
4. **Long press BtnA** (1 second) to start scanning
5. Watch devices appear on the display and in the logs

---

## Features

### BLE Scanning & Analysis

- **Passive BLE scanning** — discovers nearby devices with signal strength (RSSI) and estimated distance
- **Device info extraction** — retrieves names, service UUIDs, and manufacturer-specific data
- **GATT connections** — optionally connects to devices and reads standard BLE services (Device Info, Battery, Heart Rate, Temperature, Generic Access, TX Power, and more)

### Privacy Heuristics

- **Rotating MAC detection** — flags devices using Resolvable Private Addresses (RPA), which is a good privacy practice
- **Cleartext detection** — flags devices leaking unencrypted identifiers, which is a privacy risk
- **Exposure classification** — rates devices into tiers (None, Passive, Active, Consent) with a privacy score

### Security Analysis

- Detects **writable characteristics** (potential for unauthorized writes)
- Identifies **DFU** and **UART** services (expanded attack surface)
- Checks for **encryption** on sensitive services
- Builds **device fingerprints** from advertised UUIDs and GATT profiles

### Known Device Detection

- **Flipper Zero** — detected by known service UUIDs
- **CatHack / Apple Juice** — BLE spam tool detection
- **LightBlue** — app-based BLE testing tool
- **PwnBeacon / Pwnagotchi** — detects and reads PwnGrid beacons (identity, face, pwnd counters)

### Manufacturer Identification

Decodes manufacturer data for Apple, Google, Samsung, Epson, and more. Also parses **iBeacon** advertisements (UUID, major, minor, TX power, distance).

### GPS & Wardriving

- **Dual GPS support** — Grove UART and LoRa cap GPS
- **WiGLE CSV export** — log devices with location for mapping and analysis

### Logging

All findings are logged to three channels simultaneously:

| Channel | Details |
|---------|---------|
| **Serial** | 115200 baud, structured output |
| **SD card** | Per-category log files |
| **Web dashboard** | Real-time via WiFi AP and WebSocket |

Logs are organized into categories: Scan, GATT, Privacy, Security, Beacon, Control, GPS, System, Target, and Notify. Each device gets a **session ID** for cross-log correlation.

### XP System

GhostBLE gamifies the scanning process with experience points:

| Event | XP |
|-------|-----|
| Device discovered | +1 |
| Manufacturer data decoded | +2 |
| iBeacon parsed | +3 |
| GATT connection success | +5 |
| Characteristic subscription | +10 |
| PwnBeacon detected | +10 |
| Suspicious target found | +20 |

XP is persisted to the SD card and shown on the display.

---

## Controls

| Input | Action |
|-------|--------|
| **Long press BtnA** (1s) | Toggle BLE scanning on/off |
| **FN** | Toggle WiFi AP and web server |
| **Tab** | Toggle wardriving mode |
| **Del** | Switch GPS source (Grove / LoRa) |

### Web Interface

1. Press **FN** to enable WiFi
2. Connect to WiFi AP **`GhostBLE`** (password: **`ghostble123!`**)
3. Open **`192.168.4.1`** in a browser
4. View real-time device discovery logs via WebSocket

### Display

The LCD shows Nibbles with context-sensitive expressions (happy when scanning, sad on connection failures, thug life on suspicious finds) along with device counters and a WiFi status indicator.

---

## Hardware Requirements

- **M5Stack Cardputer** (ESP32-S3, built-in keyboard, 1.14" LCD, speaker, USB-C)
- **microSD card** (required for logging and XP persistence)
- **GPS module** (optional — Grove UART or LoRa cap for wardriving)

---

## Building & Flashing

### Arduino IDE

1. Install the **M5Stack board package** via Board Manager
2. Select board: **M5Stack Cardputer**
3. Install the required libraries via Library Manager:
   - `M5Cardputer`, `M5Unified` — hardware abstraction
   - `NimBLE-Arduino` — BLE stack
   - `ESPAsyncWebServer`, `AsyncTCP` — web dashboard
   - `TinyGPSPlus` — GPS parsing
4. Open `GhostBLE.ino`, compile, and upload

### PlatformIO

```bash
pio run -t upload
```

The project includes a custom board definition for the Cardputer in `boards/`.

### Pre-built Binaries

Pre-compiled firmware is available in `build/m5stack.esp32.m5stack_cardputer/`:

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 \
  write_flash 0x0 build/m5stack.esp32.m5stack_cardputer/GhostBLE.ino.merged.bin
```

On macOS, the port is typically `/dev/cu.usbmodem*` instead of `/dev/ttyUSB0`.

---

## Sample Output

```
<NoName> MAC: d3:c9:9d:fa:d9:9a | Rotating: Yes | Cleartext: Yes
Trying to connect to address: d3:c9:9d:fa:d9:9a
Attribute discovery failed: d3:c9:9d:fa:d9:9a

Scan Summary:
- Spotted: 4
- Sniffed: 2
- Suspicious: 0
```

---

## Project Structure

```
GhostBLE/
├── GhostBLE.ino              # Main sketch (setup, loop, WiFi, controls)
├── platformio.ini             # PlatformIO build configuration
├── boards/                    # Custom Cardputer board definition
└── src/
    ├── analyzer/              # Exposure analysis & security scoring
    ├── config/                # Hardware config, known device UUIDs
    ├── GATTServices/          # BLE service readers (11 services)
    ├── globals/               # Shared state variables
    ├── gps/                   # GPS manager (Grove + LoRa cap)
    ├── helper/                # BLE decoders, manufacturer lookup, UI
    ├── images/                # Sprite assets for Nibbles mascot
    ├── logger/                # Unified multi-target logging
    ├── models/                # Data structures (DeviceInfo)
    ├── privacyCheck/          # MAC type detection, cleartext analysis
    ├── scanner/               # Core BLE scanning and GATT operations
    ├── sdCard/                # SD card file logging
    ├── target/                # Known device detection
    ├── wardriving/            # WiGLE CSV export
    └── xp/                    # Experience points system
```

---

## Contributing

PRs and suggestions welcome! This is a learning-focused BLE project, and your ideas are appreciated.

## License

MIT License
