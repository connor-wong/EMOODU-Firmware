#include "potentiometer_driver.h"

// =====================================================================
// Constructor
// =====================================================================
PotentiometerDriver::PotentiometerDriver(int pin,
                                         int           deltaThreshold,
                                         int           limitSelfReg,
                                         int           limitActive,
                                         int           limitOverwhelm,
                                         unsigned long windowMs,
                                         unsigned long sampleDelayMs)
  : _pin(pin),
    _deltaThreshold(deltaThreshold),
    _limitSelfReg(limitSelfReg),
    _limitActive(limitActive),
    _limitOverwhelm(limitOverwhelm),
    _windowMs(windowMs),
    _sampleDelayMs(sampleDelayMs)
{}

// =====================================================================
// begin()
// =====================================================================
void PotentiometerDriver::begin() {
  analogReadResolution(12);            // 12-bit → 0–4095 on ESP32
  pinMode(_pin, INPUT);

  _lastValue   = analogRead(_pin);     // Seed baseline – avoids a
                                       // spurious first-sample spike
  _windowStart = millis();

  Serial.printf("[POT] Driver started on pin %d "
                "(deltaThreshold=%d, window=%lums)\n",
                _pin, _deltaThreshold, _windowMs);
  Serial.printf("[POT] Limits — selfReg:%d  active:%d  overwhelm:%d\n",
                _limitSelfReg, _limitActive, _limitOverwhelm);
}

// =====================================================================
// update()  –  call every loop()
// =====================================================================
void PotentiometerDriver::update() {
  // ── Sample ──────────────────────────────────────────────────────────
  _rawValue = analogRead(_pin);
  _voltage  = _rawValue * (3.3f / 4095.0f);

  // ── Delta accumulation ──────────────────────────────────────────────
  int delta = abs(_rawValue - _lastValue);
  if (delta > _deltaThreshold) {
    _activityCount++;
  }
  _lastValue = _rawValue;

  // ── End-of-window evaluation ────────────────────────────────────────
  if (millis() - _windowStart >= _windowMs) {
    EmotionState newState = _computeState(_activityCount);

    Serial.printf("[POT] Window closed — activity: %d → %s\n",
                  _activityCount, emotionStateToString(newState));

    if (_onStateChange) {
      _onStateChange(newState, _activityCount);
    }

    _currentState  = newState;
    _activityCount = 0;
    _windowStart   = millis();
  }

  // ── Sampling pace ───────────────────────────────────────────────────
  delay(_sampleDelayMs);
}

// =====================================================================
// State accessors
// =====================================================================
EmotionState PotentiometerDriver::getState()         const { return _currentState;  }
int          PotentiometerDriver::getActivityCount() const { return _activityCount; }
int          PotentiometerDriver::getRawValue()      const { return _rawValue;      }
float        PotentiometerDriver::getVoltage()       const { return _voltage;       }

// =====================================================================
// Callback registration
// =====================================================================
void PotentiometerDriver::setOnStateChange(StateChangeCallback cb) {
  _onStateChange = cb;
}

// =====================================================================
// Private helpers
// =====================================================================
EmotionState PotentiometerDriver::_computeState(int count) const {
  if (count >= _limitOverwhelm) return EmotionState::Overwhelmed;
  if (count >= _limitActive)    return EmotionState::Active;
  if (count >= _limitSelfReg)   return EmotionState::SelfRegulating;
  return EmotionState::Calm;
}