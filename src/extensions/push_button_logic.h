#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

// GPIO-free debounce + gesture FSM for the push-button extension (active-low).

constexpr uint64_t kPushButtonDebounceUs = 25ULL * 1000ULL;
constexpr uint64_t kPushButtonLongHoldUs = 800ULL * 1000ULL;
constexpr uint64_t kPushButtonDoubleWaitUs = 350ULL * 1000ULL;
constexpr size_t kPushButtonQueueDepth = 8;

enum class PushButtonEvent : uint8_t {
  Down = 0,
  Up = 1,
  Click = 2,
  DoubleClick = 3,
  LongClick = 4,
};

struct PushButtonSample {
  bool levelHigh = true;  // raw pin: HIGH released, LOW pressed
  uint64_t us = 0;
};

enum class PushButtonPhase : uint8_t {
  Idle = 0,
  Holding = 1,        // first or lone press
  LongFired = 2,      // long_click already published; still held
  WaitSecond = 3,     // short up; waiting for double or click timeout
  HoldingSecond = 4,  // second press inside double window
};

struct PushButtonLogic {
  bool enabled = false;
  bool lastAcceptedHigh = true;
  uint64_t lastAcceptedUs = 0;

  PushButtonPhase phase = PushButtonPhase::Idle;
  uint64_t downUs = 0;
  uint64_t shortUpUs = 0;
  std::optional<PushButtonEvent> pendingGesture;
  // Gesture that must publish before the edge returned by the last accept()
  // (click flushed when a new down arrives after the double window).
  std::optional<PushButtonEvent> immediateGesture;

  // Enable or disable. Enabling records |sample|.us and treats the pin as
  // released so a held button does not publish until the next press. Disabling
  // clears state with no synthetic gestures.
  void apply(bool on, PushButtonSample sample);

  // Accept a queued edge from the ISR latch. Returns Down/Up when the edge
  // clears debounce. May set immediateGesture (publish before the edge) or
  // queue DoubleClick for tick().
  std::optional<PushButtonEvent> accept(PushButtonSample sample);

  // Pop a gesture that must precede the last accept() edge, if any.
  std::optional<PushButtonEvent> takeImmediateGesture();

  // Advance long-hold and delayed-click deadlines using |nowUs|. Returns at
  // most one gesture event per call (pending DoubleClick, LongClick, or Click).
  std::optional<PushButtonEvent> tick(uint64_t nowUs);
};
