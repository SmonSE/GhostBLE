# GhostBLE – BLE Privacy Scanner

GhostBLE is a Bluetooth Low Energy (BLE) privacy scanner built for the **M5Stack Cardputer**. It discovers nearby BLE devices, analyzes their privacy posture, and flags potential security concerns. Ideal for **security researchers, tinkerers, and educators** interested in BLE privacy, device fingerprinting, and wireless reconnaissance.

A friendly mascot named **Nibbles** guides you through the scanning process on the built-in display.

---

## Features

- **Passive BLE Scanning** – Discovers nearby BLE devices with signal strength (RSSI) and estimated distance
- **Device Info Extraction** – Retrieves local name, advertised service UUIDs, and manufacturer-specific data
- **Privacy Heuristics**
  - **Rotating MAC detection** – Flags devices using Resolvable Private Addresses (RPA)
  - **Cleartext detection** – Flags devices leaking unencrypted identifiers
- **Exposure Analysis** – Classifies devices into exposure tiers (None, Passive, Active, Consent) with a privacy score
- **GATT Connection Attempts** – Optionally connects to devices and reads standard BLE services (Device Info, Battery, Heart Rate, Temperature, Generic Access)
- **Known Device Detection** – Identifies Flipper Zero, CatHack/Apple Juice, and LightBlue devices by UUID
- **Manufacturer Identification** – Decodes manufacturer data for Apple, Google, Samsung, Epson, and more
- **Multi-Channel Logging**
  - Serial output (115200 baud)
  - SD card file logging (`/device_info.txt`)
  - Web-based real-time logging via WiFi AP and WebSocket
- **Interactive UI** – Nibbles mascot with animated expressions reacts to scan events on the LCD display

---

## Hardware Requirements

- **M5Stack Cardputer** (primary target)
  - ESP32-S3 with built-in keyboard, 1.14" LCD display, speaker, and USB-C
  - **microSD card required** – the device halts on boot without one

---

## Dependencies

Install these libraries via Arduino IDE Library Manager or PlatformIO:

| Library | Purpose |
|---------|---------|
| **M5Cardputer** | Hardware abstraction for M5Stack Cardputer |
| **M5Unified** | Unified M5Stack API |
| **NimBLE-Arduino** | BLE stack (scanning, GATT client) |
| **ESPAsyncWebServer** | Web-based log viewer |
| **AsyncTCP** | Async TCP transport for web server |
| **SD** (built-in) | SD card logging |
| **SPI** (built-in) | SPI communication |

---

## Building & Flashing

### Arduino IDE

1. Install the **M5Stack board package** in Arduino IDE (Board Manager)
2. Select board: **M5Stack Cardputer**
3. Install the libraries listed above via Library Manager
4. Open `GhostBLE.ino`
5. Compile and upload

### Pre-built Binaries

Pre-compiled firmware is available in the `build/m5stack.esp32.m5stack_cardputer/` directory:

- `GhostBLE.ino.merged.bin` – Complete firmware image (flash at offset 0x0)
- `GhostBLE.ino.bootloader.bin` – Bootloader
- `GhostBLE.ino.partitions.bin` – Partition table
- `GhostBLE.ino.bin` – Application binary

Flash the merged binary with esptool:

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x0 build/m5stack.esp32.m5stack_cardputer/GhostBLE.ino.merged.bin
```

---

## Usage

### Controls

| Input | Action |
|-------|--------|
| **Long press BtnA** (1s) | Toggle BLE scanning on/off |
| **FN key** | Toggle WiFi AP and web server |

### Workflow

1. **Boot** – Insert SD card, power on. Nibbles greets you on the display
2. **Start Scan** – Long press BtnA to begin BLE scanning
3. **Monitor** – Watch the display for device counts, or connect to the web interface for detailed logs
4. **Web Interface** – Connect to WiFi AP `ESP32-Log` (password: `12345678`), then open `192.168.4.1` in a browser
5. **Review** – Logs are saved to `/device_info.txt` on the SD card

### Display

The LCD shows:
- Nibbles mascot with context-sensitive expressions (happy when scanning, sad on connection failures, thug life when active)
- Device counters: targets connected, suspicious devices, and devices with data leakage
- WiFi status indicator

---

## Privacy Heuristics

| Check | Flag | Meaning |
|-------|------|---------|
| **Rotating** | Yes | Device uses MAC address randomization (RPA) – good privacy practice |
| **Cleartext** | Yes | Device advertises personal info in plain text – privacy risk |

### Rotating MAC Addresses

- **Rotating**: The device uses a randomized private address that changes periodically, preventing persistent tracking
- **Not Rotating**: The device uses a static or fixed MAC address that can be tracked over time

### Cleartext Data

- **Cleartext**: The advertising payload contains unencrypted, readable data (device name, services, identifiers)
- **Not Cleartext**: The advertising data is encrypted or obfuscated

### Why It Matters

- **Rotating MAC addresses** help prevent persistent tracking of devices, enhancing privacy
- **Cleartext advertising** can leak sensitive information and metadata, while **encrypted/obfuscated advertising** increases security and user privacy

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
├── GhostBLE.ino              # Main sketch (setup/loop, WiFi, controls)
├── build/                     # Pre-compiled binaries
└── src/
    ├── analyzer/              # Exposure analysis & risk scoring
    ├── config/                # Hardware config, known device UUIDs
    ├── GATTServices/          # BLE service readers (battery, device info, etc.)
    ├── globals/               # Global state variables
    ├── helper/                # BLE decoder, manufacturer lookup, UI drawing
    ├── images/                # Embedded sprite assets for Nibbles mascot
    ├── logToSerialAndWeb/     # Serial + WebSocket logging
    ├── models/                # Data structures (DeviceInfo)
    ├── privacyCheck/          # MAC type detection, cleartext analysis
    ├── scanner/               # Core BLE scanning and GATT operations
    ├── sdCard/                # SD card file logging
    └── target/                # Target device handling
```

---

## Contributing

PRs and suggestions welcome! This is a learning-focused BLE project, and your ideas are appreciated.

---

## License

MIT License
