# 🔍 GhostBLE – BLE Privacy Scanner

GhostBLE is a Bluetooth Low Energy (BLE) scanner designed for the **M5Stack** platform. It detects and analyzes nearby BLE devices, logs their properties, and assesses their privacy posture. The tool is ideal for **researchers, tinkerers, and educators** interested in BLE privacy and device fingerprinting.

---

## 📦 Features

- 🛰️ **BLE Scanning** – Discovers nearby BLE devices with signal strength (RSSI) and estimated distance.
- 🏷️ **Device Info Extraction** – Retrieves:
  - Local name (if available)
  - Advertised service UUIDs
  - Manufacturer-specific data
- 🔐 **Privacy Heuristics**
  - **Rotating MAC detection** – Flags devices using Resolvable Private Addresses (RPA)
  - **Cleartext detection** – Flags devices leaking unencrypted identifiers
- 🧠 **Optional Connection Attempts** – Tries to connect and read GATT attributes (if possible)
- 💾 **Logging to Serial + Web** – Output is sent to both Serial and optional web-based logging
- 😢 **Feedback Expression** – Displays a sad expression when a device connection or attribute discovery fails
- 📊 **Summary Report** – After each scan, the tool prints:
  - Devices spotted
  - Devices successfully sniffed (connected/read)
  - Devices flagged as suspicious (based on heuristic checks)

---

## 📸 Sample Output

🔍 <NoName> MAC: d3:c9:9d:fa:d9:9a | Rotating: ✅ | Cleartext: ❗
Trying to connect to address: d3:c9:9d:fa:d9:9a
🔒 Attribute discovery failed: d3:c9:9d:fa:d9:9a
showSadExpressionTask

📊 Scan Summary:
Spotted: 4
Sniffed: 2
Suspicious: 0


---

## 🧬 Privacy Heuristics

| 🔍 Check         | ✅ Flag | Meaning                                                                 |
|------------------|--------|-------------------------------------------------------------------------|
| **Rotating**     | ✅      | Device uses MAC address randomization (RPA), a good privacy practice   |
| **Cleartext**    | ❗      | Device advertises personal info (e.g., real name) in plain text        |

---

## 🛠️ Architecture

- `scanForDevices()` runs in the main loop and:
  - Starts a BLE scan
  - For each device:
    - Extracts address, name, RSSI, and raw payload
    - Attempts to connect and read characteristics (if allowed)
    - Evaluates privacy posture
  - Logs all details and produces a scan summary

---

## 🧾 Logged Device Info

Each spotted device logs the following:
- Name: (e.g., `JBL LIVE650BTNC-LE`)
- Address: MAC (e.g., `6c:4a:d0:d9:57:59`)
- Distance estimate (based on RSSI)
- RSSI in dBm
- Raw advertising payload (in hex)
- Detected privacy flags (e.g., rotating, cleartext)

---

## 💡 Future Ideas

- Store logs on SD card or remote server
- Add BLE advertisement parsing by type
- Flag known devices or vendors
- Visual dashboard via web server on the M5Stack

---

## 📋 Requirements

- M5Stack Core (ESP32-based)
- NimBLE-Arduino library
- PlatformIO or Arduino IDE

---

## 📎 Example Usage

Plug in your M5Stack, open the Serial Monitor, and watch as BLE devices appear, along with their privacy score. Great for analyzing your headphones, fitness trackers, or smart home gear.

---

## 🤝 Contributing

PRs and suggestions welcome! This is a learning-focused BLE project, and your ideas are appreciated.

---

## 📜 License

MIT License
