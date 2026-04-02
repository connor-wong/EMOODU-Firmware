#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// =====================================================================
// BluetoothDriver
// A reusable BLE peripheral driver for ESP32 (PlatformIO / Arduino).
//
// Usage:
//   1. Instantiate BluetoothDriver with your device name, service UUID,
//      and characteristic UUID.
//   2. Optionally register onConnect / onDisconnect callbacks.
//   3. Call begin() in setup().
//   4. Call sendJson() (or sendRaw()) whenever you want to notify the
//      connected central.
//   5. Poll isConnected() before sending to avoid dropped writes.
// =====================================================================

class BluetoothDriver {
public:
  // -------------------------------------------------------------------
  // Callback types
  // -------------------------------------------------------------------
  using ConnectCallback    = std::function<void()>;
  using DisconnectCallback = std::function<void()>;

  // -------------------------------------------------------------------
  // Constructor
  //   deviceName   – the BLE advertisement name (e.g. "EMOODU")
  //   serviceUUID  – 128-bit UUID string for the GATT service
  //   charUUID     – 128-bit UUID string for the notify/read/write
  //                  characteristic
  // -------------------------------------------------------------------
  BluetoothDriver(const char* deviceName,
                  const char* serviceUUID,
                  const char* charUUID);

  // -------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------

  // Initialise the BLE stack, create service & characteristic, and
  // start advertising.  Call once from Arduino setup().
  //   txPower – ESP_PWR_LVL_P3 by default (–3 dBm to +3 dBm range).
  void begin(esp_power_level_t txPower = ESP_PWR_LVL_P3);

  // -------------------------------------------------------------------
  // State
  // -------------------------------------------------------------------
  bool isConnected() const;

  // -------------------------------------------------------------------
  // Sending data
  // -------------------------------------------------------------------

  // Send an arbitrary null-terminated string and trigger a BLE
  // notification.  Returns false if no client is connected.
  bool sendRaw(const char* payload);

  // Holds the state for a single module slot for use in sendModulesJson.
  //   moduleId – integer ID read from the voltage divider ADC (0 = no module)
  //   state    – EmotionState cast to int (0–3)
  struct ModuleData {
    int moduleId;
    int state;
  };

  // Sends a combined JSON payload for all 4 module slots plus battery:
  //   {"battery":<n>,"modules":[{"id":<n>,"state":<n>}, ...]}
  // Returns false if no client is connected.
  // Pass MODULE_COUNT (4) elements in the modules array.
  bool sendModulesJson(const ModuleData modules[], int moduleCount, int battery);

  // -------------------------------------------------------------------
  // Event callbacks (optional – set before calling begin())
  // -------------------------------------------------------------------
  void setOnConnect(ConnectCallback cb);
  void setOnDisconnect(DisconnectCallback cb);

private:
  // -------------------------------------------------------------------
  // Internal BLEServerCallbacks subclass
  // -------------------------------------------------------------------
  class ServerCallbacks : public BLEServerCallbacks {
  public:
    explicit ServerCallbacks(BluetoothDriver* driver) : _driver(driver) {}
    void onConnect(BLEServer* pServer)    override;
    void onDisconnect(BLEServer* pServer) override;
  private:
    BluetoothDriver* _driver;
  };

  // Configuration
  const char* _deviceName;
  const char* _serviceUUID;
  const char* _charUUID;

  // BLE objects
  BLEServer*         _pServer         = nullptr;
  BLECharacteristic* _pCharacteristic = nullptr;

  // State
  volatile bool _connected = false;

  // User callbacks
  ConnectCallback    _onConnect    = nullptr;
  DisconnectCallback _onDisconnect = nullptr;

  // Called by ServerCallbacks
  void _handleConnect();
  void _handleDisconnect();
};