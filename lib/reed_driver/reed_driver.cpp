#include "reed_driver.h"

// =====================================================================
// Constructor
// =====================================================================
ReedDriver::ReedDriver(gpio_num_t pin,
                       unsigned long debounceMs,
                       unsigned long sleepDelayMs)
  : _pin(pin),
    _debounceMs(debounceMs),
    _sleepDelayMs(sleepDelayMs)
{}

// =====================================================================
// begin()
// =====================================================================
void ReedDriver::begin() {
  pinMode((uint8_t)_pin, INPUT_PULLUP);

  Serial.printf("[REED] Driver started on GPIO %d "
                "(debounce=%lums)\n", (int)_pin, _debounceMs);

  logWakeReason();

  // If we just woke via EXT0 (reed switch opened), fire the callback
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("[REED] Woke from deep sleep — magnet removed.");
    if (_onWake) _onWake();
  }
}

// =====================================================================
// update()  –  call every loop()
// =====================================================================
void ReedDriver::update() {
  // Pin LOW  = switch closed = magnet present
  bool closed = (digitalRead((uint8_t)_pin) == LOW);

  if (closed && !_closed) {
    // Falling edge — start the debounce timer
    _closedSince = millis();
    _closed      = true;
    Serial.println("[REED] Switch closed — starting debounce.");
  }

  if (!closed && _closed) {
    // Rising edge — magnet removed before debounce elapsed, cancel
    _closed = false;
    Serial.println("[REED] Switch opened before debounce — cancelled.");
  }

  // Debounce elapsed → enter deep sleep
  if (_closed && (millis() - _closedSince >= _debounceMs)) {
    _enterSleep();
  }
}

// =====================================================================
// State
// =====================================================================
bool ReedDriver::isClosed() const {
  return _closed;
}

// =====================================================================
// logWakeReason()
// =====================================================================
void ReedDriver::logWakeReason() const {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  switch (cause) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("[REED] Wake reason: EXT0 (reed switch)");    break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("[REED] Wake reason: EXT1");                   break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("[REED] Wake reason: Timer");                  break;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      Serial.println("[REED] Wake reason: Power-on / reset");       break;
    default:
      Serial.printf("[REED] Wake reason: other (%d)\n", (int)cause); break;
  }
}

// =====================================================================
// Callback registration
// =====================================================================
void ReedDriver::setOnSleep(SleepCallback cb) { _onSleep = cb; }
void ReedDriver::setOnWake(WakeCallback cb)   { _onWake  = cb; }

// =====================================================================
// _enterSleep()  –  private
// =====================================================================
void ReedDriver::_enterSleep() {
  Serial.println("[REED] Entering deep sleep...");

  if (_onSleep) _onSleep();           // let the app clean up first

  delay(_sleepDelayMs);               // flush Serial before sleep

  // Wake when the reed switch opens again (pin goes HIGH)
  esp_sleep_enable_ext0_wakeup(_pin, 1);

  esp_deep_sleep_start();             // does not return
}