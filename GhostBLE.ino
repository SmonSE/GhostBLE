#include <M5StickCPlus2.h>  // Important for the Plus 2!
#include <ArduinoBLE.h>
#include <M5Unified.h>
#include <Avatar.h>  // Ensure this is the correct path to your Avatar library
#include "src/config.h"
#include <SD.h>  // SD card library
#include <SPI.h>  // SPI for SD card
#include <vector>  // Include the vector header for dynamic arrays


std::vector<String> serviceUuids;  // Dynamic array to store service UUIDs

unsigned long lastScanTime = 0;
bool deviceFound = false;
unsigned long lastFaceUpdate = 0;
bool isIdle = false;
int top = -40;
int left = -40;

int targetFoundCount = 0; // <- Zähler für gefundene Geräte

using namespace m5avatar;

Avatar avatar;

File dataFile;  // Create a file object to store information on the SD card


// Normaly at .h File C Style at the moment
void writeDeviceInfoToSD(const String& address, const String& localName, BLEDevice& peripheral);


void setup() {
  M5.begin();
  Serial.begin(115200);  // <--- ADD THIS to open Serial Monitor
  delay(500);            // <--- Give Serial Monitor time to open
  
  Serial.println("GhostBLE starting...");

  #if defined(STICK_C_PLUS2)
    M5.Lcd.setRotation(3);
  #endif // STICK_C_PLUS2

  #if defined(CARDPUTER)
  M5.Lcd.setRotation(1);
  #endif // CARDPUTER

  M5.Lcd.fillScreen(BLACK);

  // Set the face to Neutral at the start
  avatar.init(); // Initialize the avatar
  avatar.setPosition(top, left);
  avatar.setScale(0.8f);  // Set smaller avatar size for the correct display size
  avatar.setExpression(Expression::Neutral);
  isIdle = true;

  if (!BLE.begin()) {
    M5.Lcd.println("Starting BLE failed!");
    Serial.println("BLE initialization failed!");
    while (1);  // Halt the program if BLE fails to initialize
  }

  #if defined(CARDPUTER)
  // Initialize the SD card
  if (!SD.begin(SD_CS_PIN)) {  // Replace SD_CS_PIN with the correct pin number
    Serial.println("SD card initialization failed!");
    while (1);  // Halt the program if SD initialization fails
  }
  Serial.println("SD card initialized.");
  
  // Open or create a file to store device info
  dataFile = SD.open("/device_info.txt", FILE_APPEND);  // File wird fortlaufend geschrieben

  if (!dataFile) {
    Serial.println("Error opening device_info.txt for writing.");
    while (1);  // Halt the program if file opening fails
  }
  Serial.println("device_info.txt opened successfully.");
  #endif // CARDPUTER

  Serial.println("BLE initialized successfully.");
  delay(5000);
}

void loop() {
  M5.update();

  unsigned long currentTime = millis();
  
  avatar.setPosition(top, left);

  // Update the face every second
  if (currentTime - lastFaceUpdate > 1000) {
    if (isIdle) {
      avatar.setExpression(Expression::Sleepy);  // Display sleepy face when idle
    } 
    lastFaceUpdate = currentTime;
  }

  // Check if it's time to scan
  if (isIdle && !deviceFound) {
    if (currentTime - lastScanTime > SCAN_INTERVAL_MS) {
      scanForDevices();
      lastScanTime = currentTime;  // Update last scan timestamp
    }
  }
  else {
      avatar.setExpression(Expression::Angry);  // Display angry face when scanning
      BLE.stopScan();  // Stop scanning when face is angry

      delay(20000);
      isIdle = true;
      deviceFound = false;
  }
}

void scanForDevices() {
  deviceFound = false;

  BLE.scan();  // Start scanning
  BLEDevice peripheral = BLE.available();

  while (peripheral) {

    String localName = "";
    String address = "";
    int rssi = 0;

    // Check advertised Service UUIDs
    int advServiceCount = peripheral.advertisedServiceUuidCount();
    if (advServiceCount > 0) {
      for (int i = 0; i < advServiceCount; i++) {
        String serviceUuid = peripheral.advertisedServiceUuid(i);
        Serial.print("📦 Advertised Service UUID: ");
        Serial.println(serviceUuid);

        localName = peripheral.localName();
        address = peripheral.address();
        rssi = peripheral.rssi();

        Serial.print("🔎 Adresse: ");
        Serial.println(address);
        Serial.print("📛 Local Name: ");
        Serial.println(localName);

        Serial.print("📶 RSSI: ");
        Serial.println(rssi);
    
        float distance = pow(10, (-69 - rssi) / 20.0);
        Serial.print("📏 Distanz: ");
        Serial.print(distance, 2);
        Serial.println(" m");
      }
    } else {
      Serial.println("⚠ No Service UUIDs found in advertisement!");

      // Try connecting to discover services
      Serial.println("🔗 Trying to connect for service discovery...");
      if (peripheral.connect()) {
        if (peripheral.discoverAttributes()) {
          Serial.println("✅ Connected and discovered attributes!");

          avatar.setExpression(Expression::Happy);  // Display Doubt face when scanning
          delay(2000);

          // Discovery and storage of UUIDs
          for (int i = 0; i < peripheral.serviceCount(); i++) {
            localName = peripheral.localName();
            address = peripheral.address();
            rssi = peripheral.rssi();

            BLEService service = peripheral.service(i);
            String serviceUuid = service.uuid();  // Get the service UUID as a String
            serviceUuids.push_back(serviceUuid);  // Add UUID to vector
            String serviceNames = getServiceName(serviceUuid);
            Serial.print("📦 Discovered Service UUID: ");
            Serial.print(serviceUuid);
            Serial.print(" (");
            Serial.print(serviceNames);
            Serial.println(")");

            Serial.print("🔎 Adresse: ");
            Serial.println(address);
            Serial.print("📛 Local Name: ");
            Serial.println(localName);

            if (peripheral.hasManufacturerData()) {
              uint8_t mfgData[64];
              int mfgDataLen = peripheral.manufacturerData(mfgData, sizeof(mfgData));
              if (mfgDataLen >= 2) {  // Must have at least 2 bytes for ID
                // Extract manufacturer ID (Little Endian format!)
                uint16_t manufacturerId = mfgData[1] << 8 | mfgData[0];
                String manufacturerName = getManufacturerName(manufacturerId);
            
                Serial.print("🏭 Manufacturer ID: 0x");
                Serial.print(manufacturerId, HEX);
                Serial.print(" (");
                Serial.print(manufacturerName);
                Serial.println(")");
              }
            }

            Serial.print("📶 RSSI: ");
            Serial.println(rssi);
        
            float distance = pow(10, (-69 - rssi) / 20.0);
            Serial.print("📏 Distanz: ");
            Serial.print(distance, 2);
            Serial.println(" m");
            Serial.println("-------------------------------");
          }
          // Write UUID and info to SD card if connected
          writeDeviceInfoToSD(address, localName, peripheral);

        } else {
          Serial.println("❌ Attribute discovery failed.");
          avatar.setExpression(Expression::Sleepy);  // Display Doubt face when scanning
          delay(100);
        }
        peripheral.disconnect();
        avatar.setExpression(Expression::Sleepy);  // Display Doubt face when scanning
        delay(100);
      } else {
        Serial.println("❌ Connection failed.");
        avatar.setExpression(Expression::Sleepy);  // Display Doubt face when scanning
        delay(100);
      }
    }

    Serial.println("###############################");

    // Check if it’s a target device
    if (isTargetDevice(localName, address, peripheral)) {
      deviceFound = true;
      Serial.println("🎯 !!! Target device detected !!!");
      Serial.println("-------------------------------");
      isIdle = false;

      return;
    }

    peripheral = BLE.available();
  }

  Serial.print("# Hits: ");
  Serial.println(targetFoundCount);
  Serial.println("\n\n");

  isIdle = true;
}


String getManufacturerName(uint16_t manufacturerId) {
  if (manufacturerId == 0x02E5) return "Espressif Systems (M5Stack)";
  if (manufacturerId == 0x004C) return "Apple, Inc.";
  if (manufacturerId == 0x0006) return "Microsoft Corporation";
  if (manufacturerId == 0x000F) return "Broadcom Corporation";
  if (manufacturerId == 0x0131) return "Google";
  if (manufacturerId == 0x0075) return "Samsung Electronics Co.";
  if (manufacturerId == 0x00E0) return "Nintendo Co., Ltd.";
  if (manufacturerId == 0x0001) return "Ericsson Technology Licensing";
  if (manufacturerId == 0x0002) return "Intel Corp.";
  if (manufacturerId == 0x0003) return "IBM Corp.";
  if (manufacturerId == 0x0004) return "Toshiba Corp.";
  if (manufacturerId == 0x0005) return "3Com";
  if (manufacturerId == 0x0012) return "Matsushita Electric Industrial Co.";
  if (manufacturerId == 0x001D) return "Motorola";
  if (manufacturerId == 0x0025) return "Nokia Mobile Phones";
  if (manufacturerId == 0x003D) return "Hitachi, Ltd";
  if (manufacturerId == 0x0065) return "Sony Ericsson Mobile Communications";
  if (manufacturerId == 0x0079) return "LG Electronics";
  if (manufacturerId == 0x00A0) return "Qualcomm Inc.";
  if (manufacturerId == 0x00C7) return "Garmin International, Inc.";
  if (manufacturerId == 0x00D2) return "GoPro, Inc.";
  if (manufacturerId == 0x00E1) return "Bosch Sensortec GmbH";
  if (manufacturerId == 0x00EC) return "Sony Corporation";
  if (manufacturerId == 0x0106) return "Tile, Inc.";
  if (manufacturerId == 0x0110) return "Fitbit, Inc.";
  if (manufacturerId == 0x012D) return "Signify (Philips Lighting B.V.)";
  if (manufacturerId == 0x0133) return "Facebook Technologies, LLC";
  if (manufacturerId == 0x0165) return "Xiaomi Inc.";
  if (manufacturerId == 0x0171) return "OPPO Mobile Telecommunications Corp.";
  if (manufacturerId == 0x017C) return "Huawei Technologies Co., Ltd.";
  if (manufacturerId == 0x018B) return "vivo Mobile Communication Co., Ltd.";
  if (manufacturerId == 0x0195) return "OnePlus Electronics Corp.";
  
  return "Unknown Manufacturer";
}



// MOVE DO SEPARAT CLASS FROM HERE TO

// Hilfsfunktion: UUID zu Service Name übersetzen
String getServiceName(const String& uuid) {
  if (uuid == "1800") return "Generic Access Service";
  if (uuid == "1801") return "Generic Attribute Service";
  if (uuid == "180F") return "Battery Service";
  if (uuid == "180D") return "Heart Rate Service";
  if (uuid == "180A") return "Device Information Service";
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

void writeDeviceInfoToSD(const String& address, const String& localName, BLEDevice& peripheral) {
  if (dataFile) {
    dataFile.print("Device Address: ");
    dataFile.println(address);

    if (localName.length() > 0) {
      dataFile.print("Local Name: ");
      dataFile.println(localName);
    } else {
      dataFile.println("Local Name: (no name)");
    }

    // RSSI hinzufügen
    int rssi = peripheral.rssi();
    dataFile.print("RSSI: ");
    dataFile.println(rssi);

    // Manufacturer Data auslesen
    if (peripheral.hasManufacturerData()) {
      uint8_t mfgData[64];  // Buffer für Manufacturer Data (max 64 Bytes sollten reichen)
      int mfgDataLen = peripheral.manufacturerData(mfgData, sizeof(mfgData));
      if (mfgDataLen > 0) {
        dataFile.print("Manufacturer Data: ");
        for (int i = 0; i < mfgDataLen; i++) {
          if (mfgData[i] < 16) dataFile.print("0");  // führende Null für schönere Darstellung
          dataFile.print(mfgData[i], HEX);
        }
        dataFile.println();
      }
    }

    // Advertised Services auflisten
    int advServiceCount = peripheral.advertisedServiceUuidCount();
    if (advServiceCount > 0) {
      dataFile.println("Advertised Services:");
      for (int i = 0; i < advServiceCount; i++) {
        String serviceUuid = peripheral.advertisedServiceUuid(i);
        String serviceName = getServiceName(serviceUuid);
        dataFile.print("UUID: ");
        dataFile.print(serviceUuid);
        dataFile.print(" (");
        dataFile.print(serviceName);
        dataFile.println(")");
      }
    } else {
      // Write the discovered service UUIDs to the SD card
      if (dataFile) {
        dataFile.println("Discovered Services:");
        for (size_t i = 0; i < serviceUuids.size(); i++) {
          dataFile.print("Service UUID: ");
          dataFile.println(serviceUuids[i]);

          dataFile.print("Device Address: ");
          dataFile.println(address);
      
          if (localName.length() > 0) {
            dataFile.print("Local Name: ");
            dataFile.println(localName);
          } else {
            dataFile.println("Local Name: (no name)");
          }
      
          if (peripheral.hasManufacturerData()) {
            uint8_t mfgData[64];
            int mfgDataLen = peripheral.manufacturerData(mfgData, sizeof(mfgData));
            if (mfgDataLen >= 2) {  // Must have at least 2 bytes for ID
              // Extract manufacturer ID (Little Endian format!)
              uint16_t manufacturerId = mfgData[1] << 8 | mfgData[0];
              String manufacturerName = getManufacturerName(manufacturerId);
          
              dataFile.print("🏭 Manufacturer ID: 0x");
              dataFile.print(manufacturerId, HEX);
              dataFile.print(" (");
              dataFile.print(manufacturerName);
              dataFile.println(")");
            }
          }
          
          // RSSI hinzufügen
          int rssi = peripheral.rssi();
          dataFile.print("RSSI: ");
          dataFile.println(rssi);
                
          float distance = pow(10, (-69 - rssi) / 20.0);
          dataFile.print("📏 Distanz: ");
          dataFile.print(distance, 2);
          dataFile.println(" m");
        }
        dataFile.println("-------------------------------");
        dataFile.flush();  // Ensure data is written to the SD card
        Serial.println("✅ Service UUIDs written to SD card.");
        serviceUuids.clear();  // Löscht alle Elemente im Vektor
      } else {
        Serial.println("❌ Failed to write to SD card.");
      }
    }

    dataFile.println("-------------------------------");
    dataFile.flush();  // 💾 WICHTIG: Sofort auf die SD Karte schreiben
    Serial.println("✅ Data written to SD card.");
    serviceUuids.clear();  // Löscht alle Elemente im Vektor
  } else {
    Serial.println("❌ Failed to write to SD card.");
  }
}

// TO HERE 

bool isTargetDevice(String name, String address, BLEDevice peripheral) {
  // Prüfe auf spezielle bekannte MAC-Adresse
  // Vielleicht wenn er eine auffällige MAC erkennt kann er sie zu der BAD Liste hinzufügen 
  //if (address == "b0:81:84:96:a0:c9") {
  //  Serial.println("🎯 Target erkannt über MAC!");
  //  return true;
  //}

  //Optionally: check for generic/empty names
  //Some Nemos might show no name or very generic names like "ESP32" or "N/A"
  //name.toLowerCase();
  //if (name == "esp32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0") {
  //  Serial.print("Detected Name: ");
  //  Serial.println(name);
  //  Serial.println("⚠ Detected device with generic or no name. Possible M5 HW?");
  //  return true;
  //}

  // Prüfe auf die spezielle Service UUID 128-bit
  int advServiceCount = peripheral.advertisedServiceUuidCount();
  if (advServiceCount > 0) {
    for (int i = 0; i < advServiceCount; i++) {
      String serviceUuid = peripheral.advertisedServiceUuid(i);
      serviceUuid.toLowerCase();
      //c198185c-3489-d1a7-9edd-beab2fdeb783  // NEMO
      //00000020-5749-5448-0037-0024e4e9e46d  // WITHINGS Uhr HR 60
      if (serviceUuid == "c198185c-3489-d1a7-9edd-beab2fdeb783") {
        Serial.println("🎯 Target erkannt über spezielle 128-bit Service UUID!");
        return true;
      }
      // Bruce Keyboard Hack
      if (serviceUuid == "1812") {
        Serial.println("🎯 Target erkannt über spezielle 16-bit Service UUID!");
        return true;
      }
    }
  }

  return false;
}
