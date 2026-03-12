#ifndef PWNBEACON_H
#define PWNBEACON_H

#include <Arduino.h>
#include <NimBLEDevice.h>

// --- PwnBeacon Service UUIDs ---
#define PWNBEACON_SERVICE_UUID        "b34c0000-dead-face-1337-c0deba5e0001"
#define PWNBEACON_IDENTITY_CHAR_UUID  "b34c0000-dead-face-1337-c0deba5e0002"
#define PWNBEACON_FACE_CHAR_UUID      "b34c0000-dead-face-1337-c0deba5e0003"
#define PWNBEACON_NAME_CHAR_UUID      "b34c0000-dead-face-1337-c0deba5e0004"
#define PWNBEACON_SIGNAL_CHAR_UUID    "b34c0000-dead-face-1337-c0deba5e0005"

// --- Protocol constants ---
#define PWNBEACON_PROTOCOL_VERSION  0x01
#define PWNBEACON_ADV_MAX_NAME_LEN  8
#define PWNBEACON_FINGERPRINT_LEN   6
#define PWNBEACON_MAX_PEERS         16
#define PWNBEACON_PEER_TIMEOUT_MS   120000  // 2 minutes

// --- Advertisement flags ---
#define PWNBEACON_FLAG_ADVERTISE    0x01
#define PWNBEACON_FLAG_CONNECTABLE  0x02

// --- Advertisement packet (compact binary, max 21 bytes) ---
typedef struct __attribute__((packed)) {
  uint8_t  version;
  uint8_t  flags;
  uint16_t pwnd_run;
  uint16_t pwnd_tot;
  uint8_t  fingerprint[PWNBEACON_FINGERPRINT_LEN];
  uint8_t  name_len;
  char     name[PWNBEACON_ADV_MAX_NAME_LEN];
} pwnbeacon_adv_t;

// --- Peer data ---
struct PwnBeaconPeer {
  String   name;
  String   face;
  String   identity;
  uint16_t pwnd_run;
  uint16_t pwnd_tot;
  int8_t   rssi;
  uint32_t last_seen;
  bool     gone;
  bool     full_data;
  uint8_t  fingerprint[PWNBEACON_FINGERPRINT_LEN];
};

class PwnBeaconManager {
public:
  PwnBeaconManager();

  // Initialize GATT server and advertising
  void begin(const char* name, const char* identity);
  void end();

  // Update advertisement data and restart advertising
  void advertise();
  void stopAdvertise();

  // Set local state
  void setFace(const char* face);
  void setPwnd(uint16_t run, uint16_t tot);

  // Called from the existing scan loop to check a discovered device
  bool parseAdvertisedDevice(const NimBLEAdvertisedDevice* device);

  // Peer management
  void checkGonePeers();
  PwnBeaconPeer* getPeers();
  uint8_t getPeerCount();
  String getLastFriendName();

  bool isActive() const { return active; }

private:
  void computeFingerprint(const char* id, uint8_t* out);
  void buildAdvPayload(uint8_t* buf, size_t* len);
  int  findPeerByFingerprint(const uint8_t* fp);
  void parseAdvPayload(const uint8_t* data, size_t len, int8_t rssi);
  String buildIdentityJson();

  bool active;

  char     localName[PWNBEACON_ADV_MAX_NAME_LEN + 1];
  char     localIdentity[129];
  char     localFace[64];
  uint16_t localPwndRun;
  uint16_t localPwndTot;
  uint8_t  localFingerprint[PWNBEACON_FINGERPRINT_LEN];

  PwnBeaconPeer peers[PWNBEACON_MAX_PEERS];
  uint8_t  peerCount;
  String   lastFriendName;

  NimBLEServer*         bleServer;
  NimBLECharacteristic* charIdentity;
  NimBLECharacteristic* charFace;
  NimBLECharacteristic* charName;
  NimBLECharacteristic* charSignal;
};

#endif // PWNBEACON_H
