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
- 💾 **SD Card Logging** - Write all the important data to the attached SD Card on Cardputer
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
- Spotted: 4
- Sniffed: 2
- Suspicious: 0


---

## 🧬 Privacy Heuristics

| 🔍 Check         | ✅ Flag | Meaning                                                                 |
|------------------|--------|-------------------------------------------------------------------------|
| **Rotating**     | ✅      | Device uses MAC address randomization (RPA), a good privacy practice   |
| **Cleartext**    | ❗      | Device advertises personal info (e.g., real name) in plain text        |

## 🔄 Rotating vs. Cleartext

### 🔁 Rotating

This indicates whether the device’s MAC address is rotating (changing) frequently.

- ✅ **Rotating**: The device uses a **randomized or private address** that changes periodically to protect user privacy and avoid tracking.
- ❌ **Not Rotating**: The device uses a **static or fixed MAC address**.

### 🔓 Cleartext

This refers to whether the data broadcast by the device is in cleartext (unencrypted, readable) or obfuscated/encrypted.

- ❗ **Cleartext**: The advertising payload contains **unencrypted data** that anyone can read. This could include device name, services, or other openly broadcast info.
- ✔️ **Not Cleartext**: The advertising data is **encrypted, obfuscated, or otherwise protected**.

### 🛡️ Why It Matters

- **Rotating MAC addresses** help prevent persistent tracking of devices, enhancing privacy.
- **Cleartext advertising** can leak sensitive information and metadata, while **encrypted/obfuscated advertising** increases security and user privacy.


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
