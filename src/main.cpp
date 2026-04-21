#include <Arduino.h>
#include <WiFi.h>

/* Custom Drivers */
#include <bluetooth_driver.h>
#include <fsr_driver.h>
#include <potentiometer_driver.h>
#include <emotion_state.h>
#include <module_config.h>
#include <reed_driver.h>

// ── Debug Flags ──────────────────────────────────────────────────────────────
#define DEBUG 1

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
  bool         connected     = false;       // true when a module is physically present
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
  // Two 200kΩ resistors → V_out = V_bat / 2
  // LiPo: 3.0V (empty) → ADC 1861,  4.2V (full) → ADC 2606
  int raw = analogRead(BATTERY_PIN);
  return constrain(map(raw, 1861, 2606, 0, 100), 0, 100);
}

// =====================================================================
// Module detection
// =====================================================================

// Resolve a raw ADC reading to a table entry (or null if no match)
static const ModuleDefinition* resolveModule(int adc) {
  for (int t = 0; t < MODULE_TABLE_SIZE; t++) {
    if (adc >= MODULE_TABLE[t].adcMin && adc <= MODULE_TABLE[t].adcMax) {
      return &MODULE_TABLE[t];
    }
  }
  return nullptr;
}

// Initialise or teardown a single slot based on a table entry.
// Pass nullptr to clear the slot (module removed).
static void applySlot(int i, const ModuleDefinition* def, int adc) {
  // Free any existing driver
  delete slots[i].fsr;  slots[i].fsr = nullptr;
  delete slots[i].pot;  slots[i].pot = nullptr;

  if (def == nullptr) {
    slots[i].module    = FidgetModule::None;
    slots[i].sensor    = SensorType::None;
    slots[i].name      = "none";
    slots[i].connected = false;
    slots[i].state     = EmotionState::Calm;
    Serial.printf("[SLOT %d] Module removed\n", i);
    return;
  }

  slots[i].module    = def->module;
  slots[i].sensor    = def->sensor;
  slots[i].name      = def->name;
  slots[i].connected = true;
  slots[i].state     = EmotionState::Calm;

  switch (def->sensor) {
    case SensorType::Potentiometer:
      slots[i].pot = new PotentiometerDriver(SENSOR_PINS[i]);
      slots[i].pot->begin();
      Serial.printf("[SLOT %d] %s → Potentiometer (ADC=%d)\n", i, def->name, adc);
      break;

    case SensorType::FSR:
      slots[i].fsr = new FsrDriver(SENSOR_PINS[i]);
      slots[i].fsr->begin();
      Serial.printf("[SLOT %d] %s → FSR (ADC=%d)\n", i, def->name, adc);
      break;

    default:
      Serial.printf("[SLOT %d] No module detected (ADC=%d)\n", i, adc);
      break;
  }
}

// Scan all slots once — used at boot and after wake
static void identifyModules(void) {
  for (int i = 0; i < MODULE_COUNT; i++) {
    int adc = analogRead(ID_PINS[i]);
    applySlot(i, resolveModule(adc), adc);
  }
}

// Poll all ID pins and re-apply only slots that changed.
// Should be called periodically in loop().
static void pollModuleChanges(void) {
  for (int i = 0; i < MODULE_COUNT; i++) {
    int adc = analogRead(ID_PINS[i]);
    const ModuleDefinition* def = resolveModule(adc);

    FidgetModule detected = def ? def->module : FidgetModule::None;

    if (detected == slots[i].module) continue;  // no change

    applySlot(i, def, adc);

    // Immediately notify the app of the change
    if (ble.isConnected()) {
      BluetoothDriver::ModuleData moduleData[MODULE_COUNT];
      for (int j = 0; j < MODULE_COUNT; j++) {
        moduleData[j].moduleId = static_cast<int>(slots[j].module);
        moduleData[j].state    = emotionStateToId(slots[j].state);
      }
      ble.sendModulesJson(moduleData, MODULE_COUNT, batteryLevel);

      // Sync lastSentState so sendIfChanged doesn't double-fire
      for (int j = 0; j < MODULE_COUNT; j++) {
        slots[j].lastSentState = slots[j].state;
      }
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

  if(!DEBUG) {
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
  }

  // ── BLE ───────────────────────────────────────────────────────────
  ble.setOnConnect([]()    { Serial.println("[BLE] App connected.");    });
  ble.setOnDisconnect([]() { Serial.println("[BLE] App disconnected."); });
  ble.begin(ESP_PWR_LVL_N9);

  if(!DEBUG) {
    reed.begin();
  }

  // ── Detect and initialise modules ─────────────────────────────────
  identifyModules();
}

// =====================================================================
// Loop
// =====================================================================
void loop() {
  // ── Reed switch (may enter deep sleep — keep first in loop)
  if(!DEBUG) {
    reed.update();
  }

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

  // ── Hot-plug detection (every 500ms) ─────────────────────────────────
  static unsigned long lastSlotPoll = 0;
  if (millis() - lastSlotPoll >= 500) {
    lastSlotPoll = millis();
    pollModuleChanges();
  }

  // ── Battery reading (every 30 seconds) ──────────────────────────────
  static unsigned long lastBatRead = 0;
  if (millis() - lastBatRead >= 30000) {
    lastBatRead  = millis();
    batteryLevel = readBatteryPercent();
  }

  // Serial.println(readBatteryPercent());
}