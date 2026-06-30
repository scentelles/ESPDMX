#include "ble_midi.h"
#include <NimBLEDevice.h>

static NimBLEUUID PEDAL_SERVICE_UUID("03b80e5a-ede8-4b33-a751-6ce34ec4c700");
static NimBLEUUID PEDAL_CHAR_UUID   ("7772e5db-3868-4112-a1a9-f2669d106bf3");
static NimBLEUUID HID_SERVICE_UUID((uint16_t)0x1812);

static const char* PEDAL_NAME_1 = "CHOCOLATE";
static const char* PEDAL_NAME_2 = "M-Wave";

NimBLEClient*  g_client    = nullptr;
NimBLEAddress  g_pedalAddr;
bool           g_havePedal = false;
bool           g_doConnect = false;
bool           g_connected = false;
bool           g_stopScanning = false;

BleMidiCallback g_callback = nullptr;

void scanCompleteCB(NimBLEScanResults results);

void decodeBleMidi(uint8_t* data, size_t length) {
  // A CC message is 3 bytes: Status(0xB0-0xBF), CC Number(0-127), Value(0-127)
  // Since BLE MIDI packets have timestamps interspersed, we just scan for Status bytes.
  for (size_t i = 0; i < length; i++) {
    uint8_t b = data[i];
    if ((b & 0xF0) == 0xB0) { // Control Change on any channel
      if (i + 2 < length) {
        uint8_t ccId = data[i + 1];
        uint8_t val  = data[i + 2];
        // Ensure the next two bytes are actually data bytes (MSB must be 0)
        if ((ccId & 0x80) == 0 && (val & 0x80) == 0) {
          if (g_callback) {
            g_callback(ccId, val);
          }
          i += 2; // Skip the data bytes
        }
      }
    }
  }
}

void pedalNotifyCB(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t length, bool isNotify) {
  decodeBleMidi(data, length);
}

class MyClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    Serial.println("[BLE] Connecte au pedalier");
    g_connected = true;
    pClient->updateConnParams(200, 200, 0, 200);
  }

  void onDisconnect(NimBLEClient* pClient) override {
    Serial.println("[BLE] Deconnecte");
    g_connected = false;
    g_doConnect = false;

    NimBLEScan* scan = NimBLEDevice::getScan();
    if (!scan->isScanning()) {
      Serial.println("[BLE] Relance du scan...");
      g_havePedal = false;
      scan->start(3000, scanCompleteCB, false);
    }
  }
};

bool connectToPedal() {
  if (!g_client) {
    g_client = NimBLEDevice::createClient();
    g_client->setClientCallbacks(new MyClientCallbacks(), false);
  }

  Serial.print("[BLE] Tentative de connexion a : ");
  Serial.println(g_pedalAddr.toString().c_str());

  g_client->setConnectionParams(200, 200, 0, 200);

  if (!g_client->connect(g_pedalAddr)) {
    Serial.println("[BLE] Echec connexion");
    return false;
  }

  Serial.println("[BLE] Connexion OK, recherche du service...");
  NimBLERemoteService* pedalService = g_client->getService(PEDAL_SERVICE_UUID);

  if (!pedalService) {
    Serial.println("[BLE] Service non trouve (UUID incorrect ?)");
    g_client->disconnect();
    return false;
  }

  NimBLERemoteCharacteristic* pedalChar = pedalService->getCharacteristic(PEDAL_CHAR_UUID);
  if (!pedalChar) {
    Serial.println("[BLE] Caracteristique non trouvee");
    g_client->disconnect();
    return false;
  }

  if (pedalChar->canNotify()) {
    pedalChar->subscribe(true, pedalNotifyCB);
    Serial.println("[BLE] Abonnement notifications OK.");
    g_connected = true;
  } else {
    Serial.println("[BLE] Notifications non supportees !");
    g_client->disconnect();
    return false;
  }

  return true;
}

class MyScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* dev) override {
    bool nameMatch = false;
    std::string n = "";
    if (dev->haveName()) {
      n = dev->getName();
      if (n.find(PEDAL_NAME_1) != std::string::npos || n.find(PEDAL_NAME_2) != std::string::npos) {
        nameMatch = true;
      }
    }
    
    // Log all discovered devices to help debugging
    Serial.printf("[BLE] Discovered: %s (RSSI: %d)\n", n.c_str(), dev->getRSSI());

    bool hidMatch = dev->haveServiceUUID() && dev->isAdvertisingService(HID_SERVICE_UUID);

    if (nameMatch || hidMatch) {
      Serial.println("[BLE] Pedalier trouve !");
      g_pedalAddr = dev->getAddress();
      g_havePedal = true;
      g_doConnect = true;
      NimBLEDevice::getScan()->stop();
    }
  }
};

void scanCompleteCB(NimBLEScanResults results) {
  if (!g_havePedal && !g_stopScanning) {
    NimBLEDevice::getScan()->start(3000, scanCompleteCB, false);
  }
}

void bleMidiDeinit() {
  g_stopScanning = true;
  NimBLEDevice::getScan()->stop();
  NimBLEDevice::deinit(true);
  g_connected = false;
  g_havePedal = false;
  Serial.println("[BLE] Deinitialized and memory released.");
}

void bleMidiInit(BleMidiCallback callback) {
  g_callback = callback;
  Serial.println("[BLE] Init...");
  
  NimBLEDevice::init("ESP32-DMX");

  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new MyScanCallbacks());
  scan->setActiveScan(false); // Passive scan to save radio time
  scan->setInterval(160);  // 100ms
  scan->setWindow(16);     // 10ms (10% duty cycle, short bursts)
  scan->start(3000, scanCompleteCB, false);
}

void bleMidiLoop() {
  if (g_doConnect) {
    g_doConnect = false;
    if (!connectToPedal()) {
      NimBLEScan* scan = NimBLEDevice::getScan();
      scan->start(3000, scanCompleteCB, false);
    }
  }
}

bool isBleMidiConnected() {
  return g_connected;
}
