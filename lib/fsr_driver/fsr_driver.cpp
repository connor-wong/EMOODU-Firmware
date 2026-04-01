#include "fsr_driver.h"

// =====================================================================
// Constructor
// =====================================================================
FsrDriver::FsrDriver(int pin,
                     int thresholdOn,
                     int thresholdOff,
                     unsigned long windowMs,
                     unsigned long debounceMs,
                     int smoothSamples)
  : _pin(pin),
    _thresholdOn(thresholdOn),
    _thresholdOff(thresholdOff),
    _windowMs(windowMs),
    _debounceMs(debounceMs),
    _smoothSamples(smoothSamples)
{}

// =====================================================================
// begin()
// =====================================================================
void FsrDriver::begin() {
  analogSetAttenuation(ADC_11db);  // Full 0–3.3 V range on ESP32
  pinMode(_pin, INPUT);
  _windowStart = millis();
  Serial.printf("[FSR] Driver started on pin %d "
                "(on=%d, off=%d, window=%lums)\n",
                _pin, _thresholdOn, _thresholdOff, _windowMs);
}

// =====================================================================
// update()  –  call every loop()
// =====================================================================
void FsrDriver::update() {
  _rawValue = _readSmoothed();
  const unsigned long now = millis();

  // ── Squeeze detection (hysteresis + debounce) ──────────────────────
  if (!_isSqueezed && _rawValue >= _thresholdOn) {
    if (now - _lastSqueezeTime > _debounceMs) {
      _squeezeCount++;
      _isSqueezed     = true;
      _lastSqueezeTime = now;
      Serial.printf("[FSR] Squeeze #%d detected (raw=%d)\n",
                    _squeezeCount, _rawValue);
    }
  }

  if (_isSqueezed && _rawValue <= _thresholdOff) {
    _isSqueezed = false;  // Released
  }

  // ── End-of-window evaluation ────────────────────────────────────────
  if (now - _windowStart >= _windowMs) {
    EmotionState newState = _computeState(_squeezeCount);

    Serial.printf("[FSR] Window closed — squeezes: %d → %s\n",
                  _squeezeCount, emotionStateToString(newState));

    if (_onStateChange) {
      _onStateChange(newState, _squeezeCount);
    }

    _currentState = newState;
    _squeezeCount = 0;
    _windowStart  = now;
  }
}

// =====================================================================
// State accessors
// =====================================================================
EmotionState FsrDriver::getState() const      { return _currentState;  }
int          FsrDriver::getSqueezeCount() const { return _squeezeCount; }
bool         FsrDriver::isSqueezed() const     { return _isSqueezed;   }
int          FsrDriver::getRawValue() const    { return _rawValue;      }

// =====================================================================
// Callback registration
// =====================================================================
void FsrDriver::setOnStateChange(StateChangeCallback cb) {
  _onStateChange = cb;
}

// =====================================================================
// Private helpers
// =====================================================================
int FsrDriver::_readSmoothed() const {
  int sum = 0;
  for (int i = 0; i < _smoothSamples; i++) {
    sum += analogRead(_pin);
    delay(2);
  }
  return sum / _smoothSamples;
}

EmotionState FsrDriver::_computeState(int count) const {
  if (count <= 1) return EmotionState::Calm;
  if (count <= 2) return EmotionState::SelfRegulating;
  if (count <= 3) return EmotionState::Active;
  return EmotionState::Overwhelmed;
}