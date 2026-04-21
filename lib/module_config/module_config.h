#pragma once

#include <Arduino.h>

// =====================================================================
// module_config.h
// Centralised configuration for all detachable fidget modules.
//
// Concepts:
//   FidgetModule – the physical module identity (Twister, Popper, etc.)
//   SensorType   – the sensor the module uses internally (FSR or Pot)
//
// To add a new fidget module:
//   1. Add an entry to the FidgetModule enum.
//   2. Choose a unique resistor → calculate its expected ADC value.
//   3. Add a row to MODULE_TABLE with its ADC range and sensor type.
//   That's it — main.cpp and the drivers need no changes.
// =====================================================================

// =====================================================================
// Hardware layout
// =====================================================================
#define MODULE_COUNT 4

// ADC pins used to identify which module is plugged in (voltage divider)
constexpr uint8_t ID_PINS[MODULE_COUNT]     = { 1,  2,  4,  5 };

// ADC pins connected to the module's sensor output
constexpr uint8_t SENSOR_PINS[MODULE_COUNT] = {8, 9, 10, 11};

#define REED_PIN    8
#define BATTERY_PIN 9

// =====================================================================
// Sensor types  (internal implementation detail of each module)
// =====================================================================
enum class SensorType : uint8_t {
  None          = 0,
  FSR           = 1,
  Potentiometer = 2,
  RUB           = 3, // same as FSR but with different thresholds and state mapping
  PUSH          = 4, // same as FSR but with different thresholds and state mapping
};

// =====================================================================
// Fidget module identities
// -----------------------------------------------------------------------
// Each value is sent as the "id" integer in the BLE JSON payload.
// Values must be unique and non-zero (0 = no module connected).
// =====================================================================
enum class FidgetModule : uint8_t {
  None    = 0,
  DJ_DISK = 1,  // Potentiometer
  POP_IT  = 2,  // FSR
  WAVE_PAD = 3,  // FSR
  BLOOM_BOX = 4, // Potentiometer
  PUSH_IT = 5,  // FSR
  TOM = 6,     // Potentiometer
};

// =====================================================================
// Module definition table
// -----------------------------------------------------------------------
// Maps ADC reading ranges → FidgetModule identity + its SensorType.
//
// Voltage divider formula (10k pull-up to 3.3V):
//   ADC = 4095 × R_module / (R_pullup + R_module)
//
//   R_module    Expected ADC    Suggested range
//   ─────────── ─────────────── ───────────────
//   4.7 kΩ      1310            1100 – 1500     ← texturepad1 (Done)
//   10 kΩ       2048            1700 – 2300     ← texturepad2 (Done)
//   15 kΩ       2457            2250 – 2650     ← twist1 (Done)
//   22 kΩ       2815            2500 – 3100     ← texturepad3 (Done)
//   47 kΩ       3376            3150 – 3600     ← twist2 (Done)
//   75 kΩ       3610            3450 – 3850     ← twist3 (Done)
//   No module   ~4095           outside all ranges → None
// =====================================================================
struct ModuleDefinition {
  int          adcMin;
  int          adcMax;
  FidgetModule module;
  SensorType   sensor;
  const char*  name;    // human-readable label used in Serial logs
};

constexpr ModuleDefinition MODULE_TABLE[] = {
  { 1100, 1500, FidgetModule::PUSH_IT,  SensorType::PUSH,           "pushit"  }, // 4.7 kΩ
  { 1700, 2300, FidgetModule::WAVE_PAD,  SensorType::RUB,           "wavepad"  }, // 10 kΩ
  { 2500, 3100, FidgetModule::POP_IT,  SensorType::FSR,           "popit"  }, // 22 kΩ

  { 2250, 2650, FidgetModule::BLOOM_BOX, SensorType::Potentiometer, "bloombox" }, // 15 kΩ
  { 3150, 3400, FidgetModule::DJ_DISK, SensorType::Potentiometer, "djdisk" }, // 47 kΩ
  { 3450, 3850, FidgetModule::TOM, SensorType::Potentiometer, "tom" }, // 75 kΩ
};

constexpr int MODULE_TABLE_SIZE =
    sizeof(MODULE_TABLE) / sizeof(MODULE_TABLE[0]);