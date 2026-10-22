#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

// GPIO-free debounce for the push-button extension (active-low pin levels).

constexpr uint64_t kPushButtonDebounceUs = 25ULL * 1000ULL;
constexpr size_t kPushButtonQueueDepth = 8;

enum class PushButtonEvent : uint8_t {
  Down = 0,
  Up = 1,
};

struct PushButtonSample {
  bool levelHigh = true;  // raw pin: HIGH released, LOW pressed
  uint64_t us = 0;
};

struct PushButtonLogic {
  bool enabled = false;
  bool lastAcceptedHigh = true;
  uint64_t lastAcceptedUs = 0;

  // Enable or disable. Enabling records |sample|.us and treats the pin as
  // released so a held button does not publish until the next press. Disabling
  // clears state.
  void apply(bool on, PushButtonSample sample);

  // Accept a queued edge from the ISR latch. Returns an event when the edge
  // clears debounce against the last accepted level.
  std::optional<PushButtonEvent> accept(PushButtonSample sample);
};
