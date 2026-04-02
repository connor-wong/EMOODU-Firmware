#pragma once

// =====================================================================
// emotion_state.h
// Shared emotional state definitions used across all sensor drivers.
//
// Include this file in any driver or sketch that needs EmotionState.
// Because it uses #pragma once, it is safe to include from multiple
// drivers in the same build unit without redefinition errors.
// =====================================================================

#include <Arduino.h>

// ---------------------------------------------------------------------
// EmotionState
// ---------------------------------------------------------------------
enum class EmotionState : uint8_t {
  Calm           = 0,
  SelfRegulating = 1,
  Active         = 2,
  Overwhelmed    = 3
};

// Returns a lowercase string matching the BLE JSON values expected
// by the web app ("calm", "selfregulating", "active", "overstimulated")
inline const char* emotionStateToString(EmotionState state) {
  switch (state) {
    case EmotionState::Calm:           return "calm";
    case EmotionState::SelfRegulating: return "selfregulating";
    case EmotionState::Active:         return "active";
    case EmotionState::Overwhelmed:    return "overstimulated";
    default:                           return "unknown";
  }
}

// Returns the integer ID used in BLE JSON payloads (0–3)
inline int emotionStateToId(EmotionState state) {
  return static_cast<int>(state);
}