#include <Arduino.h>
#include <WiFi.h>

/* Custom Drivers */
#include <bluetooth_driver.h>
#include <fsr_driver.h>
#include <potentiometer_driver.h>
#include <reed_driver.h>

/* Shared definitions */
#include <emotion_state.h>
#include <module_config.h>

// =====================================================================
// UUIDs
// =====================================================================
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// =====================================================================
// Per-slot runtime state
// =====================================================================
struct ModuleSlot {
  FidgetModule module        = FidgetModule::None;
  SensorType   sensor        = SensorType::None;
  const char*  name          = "none";
  EmotionState state         = EmotionState::Calm;
  EmotionState lastSentState = EmotionState::Calm;

  // Only one is non-null, determined by the module's SensorType
  FsrDriver*           fsr = nullptr;
  PotentiometerDriver* pot = nullptr;
};

static ModuleSlot slots[MODULE_COUNT];

// =====================================================================
// BLE driver
// =====================================================================
BluetoothDriver ble("EMOODU", SERVICE_UUID, CHARACTERISTIC_UUID);

// =====================================================================
// Reed switch driver
// =====================================================================
ReedDriver reed((gpio_num_t)REED_PIN);

// =====================================================================
// Battery
// =====================================================================
static int batteryLevel = 0;

int readBatteryPercent(void) {
  // 3.7V LiPo through a voltage divider scaled to 0–3.3V ADC range.
  // Adjust the scale factor to match your divider ratio.
  int raw = analogRead(BATTERY_PIN);
  return constrain(map(raw, 0, 4095, 0, 100), 0, 100);
}

// =====================================================================
// Module detection
// =====================================================================
static void identifyModules(void) {
  for (int i = 0; i < MODULE_COUNT; i++) {
    int adc = analogRead(ID_PINS[i]);

    // Look up the ADC reading in the module definition table
    FidgetModule module = FidgetModule::None;
    SensorType   sensor = SensorType::None;
    const char*  name   = "none";

    for (int t = 0; t < MODULE_TABLE_SIZE; t++) {
      if (adc >= MODULE_TABLE[t].adcMin && adc <= MODULE_TABLE[t].adcMax) {
        module = MODULE_TABLE[t].module;
        sensor = MODULE_TABLE[t].sensor;
        name   = MODULE_TABLE[t].name;
        break;
      }
    }

    slots[i].module = module;
    slots[i].sensor = sensor;
    slots[i].name   = name;

    // Free any previously allocated driver before re-allocating
    delete slots[i].fsr;  slots[i].fsr = nullptr;
    delete slots[i].pot;  slots[i].pot = nullptr;

    switch (sensor) {
      case SensorType::Potentiometer:
        slots[i].pot = new PotentiometerDriver(SENSOR_PINS[i]);
        slots[i].pot->begin();
        Serial.printf("[SLOT %d] %s → Potentiometer (ADC=%d)\n", i, name, adc);
        break;

      case SensorType::FSR:
        slots[i].fsr = new FsrDriver(SENSOR_PINS[i]);
        slots[i].fsr->begin();
        Serial.printf("[SLOT %d] %s → FSR (ADC=%d)\n", i, name, adc);
        break;

      default:
        Serial.printf("[SLOT %d] No module detected (ADC=%d)\n", i, adc);
        break;
    }
  }
}

// =====================================================================
// BLE transmission  –  send only when at least one slot changed state
// =====================================================================
static void sendIfChanged(void) {
  if (!ble.isConnected()) return;

  bool anyChanged = false;
  for (int i = 0; i < MODULE_COUNT; i++) {
    if (slots[i].state != slots[i].lastSentState) {
      anyChanged = true;
      break;
    }
  }

  if (!anyChanged) return;

  // Build the ModuleData array for the BLE driver
  BluetoothDriver::ModuleData moduleData[MODULE_COUNT];
  for (int i = 0; i < MODULE_COUNT; i++) {
    moduleData[i].moduleId = static_cast<int>(slots[i].module);
    moduleData[i].state    = emotionStateToId(slots[i].state);
  }

  if (ble.sendModulesJson(moduleData, MODULE_COUNT, batteryLevel)) {
    // Only advance lastSentState on a successful transmission
    for (int i = 0; i < MODULE_COUNT; i++) {
      slots[i].lastSentState = slots[i].state;
    }
  }
}

// =====================================================================
// Setup
// =====================================================================
void setup() {
  setCpuFrequencyMhz(80);
  Serial.begin(115200);

  // ── Power saving ──────────────────────────────────────────────────
  WiFi.mode(WIFI_OFF);
  btStop();

  // ── ADC ───────────────────────────────────────────────────────────
  analogSetAttenuation(ADC_11db);  // Full 0–3.3V range

  // ── Reed switch ───────────────────────────────────────────────────
  reed.setOnSleep([]() {
    // Cleanly shut down BLE before the ESP32 powers off
    Serial.println("[REED] Shutting down BLE before sleep...");
    BLEDevice::deinit(true);
  });

  reed.setOnWake([]() {
    // Re-identify modules in case anything was swapped while asleep
    Serial.println("[REED] Woke up — re-scanning modules...");
    identifyModules();
    ble.begin(ESP_PWR_LVL_N9);
  });

  // ── BLE ───────────────────────────────────────────────────────────
  ble.setOnConnect([]()    { Serial.println("[BLE] App connected.");    });
  ble.setOnDisconnect([]() { Serial.println("[BLE] App disconnected."); });
  ble.begin(ESP_PWR_LVL_N9);
  reed.begin();

  // ── Detect and initialise modules ─────────────────────────────────
  identifyModules();
}

// =====================================================================
// Loop
// =====================================================================
void loop() {
  // ── Reed switch (may enter deep sleep — keep first in loop)
  reed.update();

  // ── Update each active sensor driver ────────────────────────────────
  for (int i = 0; i < MODULE_COUNT; i++) {
    if (slots[i].fsr) {
      slots[i].fsr->update();
      slots[i].state = slots[i].fsr->getState();
    } else if (slots[i].pot) {
      slots[i].pot->update();
      slots[i].state = slots[i].pot->getState();
    }
  }

  // ── Send combined JSON if anything changed ───────────────────────────
  sendIfChanged();

  // ── Battery reading (every 30 seconds) ──────────────────────────────
  static unsigned long lastBatRead = 0;
  if (millis() - lastBatRead >= 30000) {
    lastBatRead  = millis();
    batteryLevel = readBatteryPercent();
  }
}