# GhostBLE – BLE Scanner für Apple-Geräte & Co.

GhostBLE ist ein auf dem M5Stack basierender Bluetooth Low Energy (BLE) Scanner, der gezielt nach BLE-Werbung (Advertising Packets) sucht. Das Projekt erkennt insbesondere Apple-Geräte wie AirTags, iPhones oder MacBooks anhand ihrer charakteristischen BLE-Werbung – auch wenn sie keine Namen aussenden.

## 🔍 Funktionsweise

GhostBLE scannt kontinuierlich nach BLE-Geräten in der Umgebung. Dabei werden folgende Informationen pro Gerät gesammelt:

- **MAC-Adresse** (z. B. `98:84:e3:22:76:c5`)
- **Gerätename** (falls vorhanden, sonst `Unknown`)
- **RSSI-Wert** (Signalstärke)
- **Geschätzte Entfernung** (basierend auf RSSI)
- **Rohdaten des Advertising Payloads**
- **Datenschutzstatus**:
  - ✅ **Rotating**: Die MAC-Adresse rotiert regelmäßig (z. B. Apple Private BLE)
  - ❗ **Cleartext**: Eindeutige MAC, keine Rotation → potenziell trackbar

## 📦 Anzeigeformat (Beispielausgabe)

```plaintext
🔍 Unknown
MAC: 6c:4a:d0:d9:57:59 | Rotating: ✅ | Cleartext: ❗
📏 Distance: 2.37 m
RSSI: -62
