#include <Arduino.h>
#include <WiFi.h>

/* Custom Drivers */
#include <bluetooth_driver.h>
#include <fsr_driver.h>
#include <potentiometer_driver.h>

// ── UUIDs ──────────────────────────────────────────────────────────────
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ── Pin ────────────────────────────────────────────────────────────────
#define MODULE_ONE_ID_PIN 1
#define MODULE_TWO_ID_PIN 2
#define MODULE_THREE_ID_PIN 4
#define MODULE_FOUR_ID_PIN 5

#define REED_PIN 6
#define BATTERY_PIN 7

#define MODULE_ONE_PIN 8
#define MODULE_TWO_PIN 9
#define MODULE_THREE_PIN 10
#define MODULE_FOUR_PIN 11

// ── Driver instances ───────────────────────────────────────────────────
BluetoothDriver ble("EMOODU", SERVICE_UUID, CHARACTERISTIC_UUID);
FsrDriver       fsr(MODULE_ONE_PIN);
PotentiometerDriver pot(MODULE_TWO_PIN);

// ── Simulated battery (replace with real ADC read later) ───────────────
int batteryLevel = 0;

// ── Setup ──────────────────────────────────────────────────────────────
void setup() {
  setCpuFrequencyMhz(80);
  Serial.begin(115200);

  // ── Power saving ────────────────────────────────────────────────────
  WiFi.mode(WIFI_OFF);   // disable WiFi radio (~50–100 mA saved at peak)
  btStop();              // disable Bluetooth Classic (keep BLE only)

  // Fire a BLE notification every time a driver detects a state change.
  fsr.setOnStateChange([](EmotionState state, int count) {
    if (ble.isConnected()) {
      // ble.sendJson((int)state, "twistknob", batteryLevel);
    }
  });

  pot.setOnStateChange([](EmotionState state, int count) {
    if (ble.isConnected()) {
      ble.sendJson((int)state, "twistknob", batteryLevel);
    }
  });

  ble.setOnConnect([]()    { Serial.println("App connected.");    });
  ble.setOnDisconnect([]() { Serial.println("App disconnected."); });

  
  ble.begin(ESP_PWR_LVL_N9);  // –9 dBm, shortest range, lowest draw 
  // ble.begin(ESP_PWR_LVL_P3);
  fsr.begin();
  pot.begin();
}

// ── Loop ───────────────────────────────────────────────────────────────
void loop() {
  fsr.update();   // handles squeeze detection, windowing, and fires the callback
  pot.update();  // handles twist detection, windowing, and fires the callback

  // Simulate battery level cycling (swap for a real read when ready)
  // static unsigned long lastBatUpdate = 0;
  // if (millis() - lastBatUpdate >= 5000) {
  //   lastBatUpdate = millis();
  //   batteryLevel  = (batteryLevel >= 100) ? 0 : batteryLevel + 10;
  // }
}