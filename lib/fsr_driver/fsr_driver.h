#pragma once

#include <Arduino.h>

// States
#include <emotion_state.h>

// =====================================================================
// FsrDriver
// A reusable Force Sensitive Resistor (FSR) driver for ESP32.
//
// Detects squeeze events from an FSR using hysteresis thresholds,
// debouncing, and optional smoothing. Reports an emotional state
// based on squeeze frequency over a configurable time window.
//
// Usage:
//   1. Instantiate FsrDriver with your pin and threshold values.
//   2. Optionally register an onStateChange callback.
//   3. Call begin() in setup().
//   4. Call update() every loop iteration.
//   5. Read getState() or getSqueezeCount() as needed.
// =====================================================================

// ---------------------------------------------------------------------
// FsrDriver
// ---------------------------------------------------------------------
class FsrDriver {
public:
  // -------------------------------------------------------------------
  // Callback type – fired at the end of each evaluation window when
  // the state changes.
  //   newState     – freshly computed emotional state
  //   squeezeCount – number of squeezes detected in the last window
  // -------------------------------------------------------------------
  using StateChangeCallback = std::function<void(EmotionState newState,
                                                  int squeezeCount)>;

  // -------------------------------------------------------------------
  // Constructor
  //   pin           – ADC pin the FSR is wired to
  //   thresholdOn   – raw ADC value at which a squeeze begins
  //   thresholdOff  – raw ADC value at which a squeeze ends (hysteresis)
  //   windowMs      – evaluation window length in ms (default 1000)
  //   debounceMs    – minimum ms between squeeze onsets (default 200)
  //   smoothSamples – number of ADC samples averaged per read (default 5)
  // -------------------------------------------------------------------
  FsrDriver(int pin,
            int thresholdOn    = 2500,
            int thresholdOff   = 2000,
            unsigned long windowMs     = 1000,
            unsigned long debounceMs   = 200,
            int smoothSamples  = 5);

  // -------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------

  // Configure the ADC and pin. Call once from Arduino setup().
  void begin();

  // Process FSR input and update internal state.
  // Must be called every loop iteration for accurate timing.
  void update();

  // -------------------------------------------------------------------
  // State accessors
  // -------------------------------------------------------------------

  // Current emotional state (updated once per window)
  EmotionState getState() const;

  // Squeeze count accumulated in the current (incomplete) window
  int getSqueezeCount() const;

  // True while the FSR is actively pressed above thresholdOff
  bool isSqueezed() const;

  // Raw smoothed ADC reading from the last update() call
  int getRawValue() const;

  // -------------------------------------------------------------------
  // Event callback (optional – set before or after begin())
  // -------------------------------------------------------------------
  void setOnStateChange(StateChangeCallback cb);

private:
  // -------------------------------------------------------------------
  // Helpers
  // -------------------------------------------------------------------
  int          _readSmoothed() const;
  EmotionState _computeState(int count) const;

  // -------------------------------------------------------------------
  // Configuration
  // -------------------------------------------------------------------
  int           _pin;
  int           _thresholdOn;
  int           _thresholdOff;
  unsigned long _windowMs;
  unsigned long _debounceMs;
  int           _smoothSamples;

  // -------------------------------------------------------------------
  // Runtime state
  // -------------------------------------------------------------------
  int           _rawValue      = 0;
  int           _squeezeCount  = 0;
  bool          _isSqueezed    = false;
  EmotionState  _currentState  = EmotionState::Calm;

  unsigned long _windowStart    = 0;
  unsigned long _lastSqueezeTime = 0;

  StateChangeCallback _onStateChange = nullptr;
};