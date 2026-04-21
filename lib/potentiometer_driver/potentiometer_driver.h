#pragma once

#include <Arduino.h>

// States
#include <emotion_state.h>

// =====================================================================
// PotentiometerDriver
// A reusable rotary potentiometer activity driver for ESP32.
//
// Tracks how much the knob moves within a fixed time window by
// counting ADC deltas that exceed a noise threshold. At the end of
// each window it maps the movement count to an EmotionState and
// fires an optional callback.
//
// Usage:
//   1. Instantiate with your pin and tuning parameters.
//   2. Optionally register an onStateChange callback.
//   3. Call begin() in setup().
//   4. Call update() every loop iteration.
//   5. Read getState(), getActivityCount(), or getRawValue() as needed.
// =====================================================================

// ---------------------------------------------------------------------
// PotentiometerDriver
// ---------------------------------------------------------------------
class PotentiometerDriver {
public:
  // -------------------------------------------------------------------
  // Callback type – fired once per window when evaluation completes.
  //   newState      – computed EmotionState for the closed window
  //   activityCount – number of significant movements detected
  // -------------------------------------------------------------------
  using StateChangeCallback = std::function<void(EmotionState newState,
                                                  int activityCount)>;

  // -------------------------------------------------------------------
  // Constructor
  //   pin             – ADC pin the potentiometer wiper is connected to
  //   deltaThreshold  – min ADC delta per sample to count as movement
  //                     (filters ADC noise), default 100
  //   limitSelfReg    – activity ticks to reach Self-regulating, default 15
  //   limitActive     – activity ticks to reach Active, default 30
  //   limitOverwhelm  – activity ticks to reach Overwhelmed, default 45
  //   windowMs        – evaluation window length in ms, default 1000
  //   sampleDelayMs   – ms to wait between samples in update(), default 10
  // -------------------------------------------------------------------
  PotentiometerDriver(int pin,
                      int           deltaThreshold = 123, // 3% of total range  
                      int           limitSelfReg   = 7,
                      int           limitActive    = 14,
                      int           limitOverwhelm = 21,
                      unsigned long windowMs       = 1000,
                      unsigned long sampleDelayMs  = 10);

  // -------------------------------------------------------------------
  // Lifecycle
  // -------------------------------------------------------------------

  // Sets ADC resolution and seeds the last-value baseline.
  // Call once from Arduino setup().
  void begin();

  // Reads the potentiometer, accumulates activity, evaluates the window
  // when due. Contains a brief blocking delay (sampleDelayMs) for
  // stable ADC reads. Call every loop().
  void update();

  // -------------------------------------------------------------------
  // State accessors
  // -------------------------------------------------------------------

  // EmotionState computed at the end of the last closed window
  EmotionState getState() const;

  // Activity ticks accumulated in the current (still open) window
  int getActivityCount() const;

  // Raw ADC value from the most recent sample (0–4095)
  int getRawValue() const;

  // Converted voltage from the most recent sample (0.0–3.3 V)
  float getVoltage() const;

  // -------------------------------------------------------------------
  // Callback (optional – register before or after begin())
  // -------------------------------------------------------------------
  void setOnStateChange(StateChangeCallback cb);

private:
  EmotionState _computeState(int count) const;

  // Configuration
  int           _pin;
  int           _deltaThreshold;
  int           _limitSelfReg;
  int           _limitActive;
  int           _limitOverwhelm;
  unsigned long _windowMs;
  unsigned long _sampleDelayMs;

  // Runtime state
  int           _rawValue      = 0;
  float         _voltage       = 0.0f;
  int           _lastValue     = 0;
  int           _activityCount = 0;
  EmotionState  _currentState  = EmotionState::Calm;
  unsigned long _windowStart   = 0;

  StateChangeCallback _onStateChange = nullptr;
};