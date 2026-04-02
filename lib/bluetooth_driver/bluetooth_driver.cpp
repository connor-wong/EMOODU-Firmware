#include "bluetooth_driver.h"

// =====================================================================
// Constructor
// =====================================================================
BluetoothDriver::BluetoothDriver(const char* deviceName,
                                 const char* serviceUUID,
                                 const char* charUUID)
  : _deviceName(deviceName),
    _serviceUUID(serviceUUID),
    _charUUID(charUUID)
{}

// =====================================================================
// begin() – initialise and start advertising
// =====================================================================
void BluetoothDriver::begin(esp_power_level_t txPower) {
  Serial.printf("[BLE] Initialising device \"%s\"...\n", _deviceName);

  BLEDevice::init(_deviceName);
  BLEDevice::setPower(txPower);

  // --- Server ---
  _pServer = BLEDevice::createServer();
  _pServer->setCallbacks(new ServerCallbacks(this));

  // --- Service ---
  BLEService* pService = _pServer->createService(_serviceUUID);

  // --- Characteristic (Read | Write | Notify) ---
  _pCharacteristic = pService->createCharacteristic(
    _charUUID,
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  // Required descriptor for BLE notifications
  _pCharacteristic->addDescriptor(new BLE2902());

  // Set an initial placeholder value
  _pCharacteristic->setValue("READY");

  pService->start();

  // --- Advertising ---
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(_serviceUUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Helps iOS / Android discovery
  pAdvertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();

  Serial.printf("[BLE] Advertising started. Device: %s\n", _deviceName);
  Serial.printf("[BLE] Service UUID : %s\n", _serviceUUID);
  Serial.printf("[BLE] Char    UUID : %s\n", _charUUID);
}

// =====================================================================
// State
// =====================================================================
bool BluetoothDriver::isConnected() const {
  return _connected;
}

// =====================================================================
// sendRaw()
// =====================================================================
bool BluetoothDriver::sendRaw(const char* payload) {
  if (!_connected || _pCharacteristic == nullptr) {
    Serial.println("[BLE] sendRaw() skipped – no client connected.");
    return false;
  }
  _pCharacteristic->setValue(payload);
  _pCharacteristic->notify();
  Serial.printf("[BLE] Sent: %s\n", payload);
  return true;
}

// =====================================================================
// sendModulesJson()
// {"battery":<n>,"modules":[{"id":<n>,"state":<n>}, ...]}
// =====================================================================
bool BluetoothDriver::sendModulesJson(const ModuleData modules[],
                                      int moduleCount,
                                      int battery) {
  if (!_connected) return false;

  // Max size: {"battery":100,"modules":[{"id":99,"state":3},{"id":99,"state":3},{"id":99,"state":3},{"id":99,"state":3}]}
  char buf[200];
  int  pos = 0;

  pos += snprintf(buf + pos, sizeof(buf) - pos,
                  "{\"battery\":%d,\"modules\":[", battery);

  for (int i = 0; i < moduleCount; i++) {
    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "{\"id\":%d,\"state\":%d}%s",
                    modules[i].moduleId,
                    modules[i].state,
                    (i < moduleCount - 1) ? "," : "");
  }

  snprintf(buf + pos, sizeof(buf) - pos, "]}");

  return sendRaw(buf);
}

// =====================================================================
// Callback registration
// =====================================================================
void BluetoothDriver::setOnConnect(ConnectCallback cb) {
  _onConnect = cb;
}

void BluetoothDriver::setOnDisconnect(DisconnectCallback cb) {
  _onDisconnect = cb;
}

// =====================================================================
// Internal event handlers (called from ServerCallbacks)
// =====================================================================
void BluetoothDriver::_handleConnect() {
  _connected = true;
  Serial.println("[BLE] Client connected.");
  if (_onConnect) _onConnect();
}

void BluetoothDriver::_handleDisconnect() {
  _connected = false;
  Serial.println("[BLE] Client disconnected. Restarting advertising...");
  BLEDevice::startAdvertising();
  if (_onDisconnect) _onDisconnect();
}

// =====================================================================
// ServerCallbacks (nested class)
// =====================================================================
void BluetoothDriver::ServerCallbacks::onConnect(BLEServer* /*pServer*/) {
  _driver->_handleConnect();
}

void BluetoothDriver::ServerCallbacks::onDisconnect(BLEServer* /*pServer*/) {
  _driver->_handleDisconnect();
}