#pragma once

#include <Arduino.h>
#include <esp_sleep.h>

// =====================================================================
// ReedDriver
// Manages deep sleep based on a magnetic reed switch for ESP32-S3.
//
// The reed switch acts as a lid/case sensor:
//   - Closed (magnet present) → LOW  → device goes to deep sleep
//   - Open   (magnet absent)  → HIGH → device stays awake
//
// Wake-up is handled by EXT0 — the same pin triggers wake when the
// magnet is removed (pin goes HIGH).
//
// Usage:
//   1. Instantiate ReedDriver with an RTC-capable GPIO pin.
//   2. Optionally register onSleep / onWake callbacks.
//   3. Call begin() in setup().
//   4. Call update() every loop() — it will trigger deep sleep
//      automatically when the switch closes.
//
// Note: After waking from deep sleep the ESP32 runs setup() again.
//       Call logWakeReason() in setup() to print why it woke up.
// =====================================================================
class ReedDriver {
public:
  // -------------------------------------------------------------------
  // Callback types
  // -------------------------------------------------------------------
  using SleepCallback = std::function<void()>;  // fired just before sleep
  using WakeCallback  = std::function<void()>;  // fired on wake in begin()

  // -------------------------------------------------------------------
  // Constructor
  //   pin          – RTC-capable GPIO the reed switch is wired to.
  //                  Must support EXT0 wakeup (GPIO 0–21 on ESP32-S3).
  //   debounceMs   – how long the switch must stay closed before sleep
  //                  triggers, to avoid false positives. Default 500ms.
  //   sleepDelayMs – additional delay after the sleep callback fires,
  //                  giving Serial time to flush. Default 200ms.
  // -------------------------------------------------------------------
  ReedDriver(gpio_num_t pin,
             unsigned long debounceMs   = 500,
             unsigned long sleepDelayMs = 200);

  // -------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------

  // Configures the pin, logs the wake reason, and fires onWake if set.
  // Call once from Arduino setup().
  void begin();

  // Reads the reed switch and enters deep sleep if the switch has been
  // closed for longer than debounceMs. Call every loop().
  void update();

  // -------------------------------------------------------------------
  // State
  // -------------------------------------------------------------------

  // True if the magnet is currently detected (switch closed, pin LOW)
  bool isClosed() const;

  // Prints the ESP32 wake reason to Serial
  void logWakeReason() const;

  // -------------------------------------------------------------------
  // Callbacks (optional — register before begin())
  // -------------------------------------------------------------------

  // Fired just before deep sleep starts — use to shut down BLE, flush
  // buffers, save state, etc.
  void setOnSleep(SleepCallback cb);

  // Fired in begin() when the wake reason is EXT0 (reed switch opened)
  void setOnWake(WakeCallback cb);

private:
  void _enterSleep();

  gpio_num_t    _pin;
  unsigned long _debounceMs;
  unsigned long _sleepDelayMs;

  bool          _closed       = false;
  unsigned long _closedSince  = 0;  // millis() when switch first closed

  SleepCallback _onSleep = nullptr;
  WakeCallback  _onWake  = nullptr;
};