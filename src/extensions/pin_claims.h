#pragma once

#include <cstddef>
#include <cstdint>

// GPIO claims among enabled Extension-port extensions. Pure logic — no NVS/GPIO.

constexpr size_t kExtPinClaimMaxPins = 4;
constexpr size_t kExtPinClaimMaxProviders = 8;

struct ExtPinClaim {
  const char* id = nullptr;
  uint8_t pins[kExtPinClaimMaxPins] = {};
  size_t pinCount = 0;
};

// Fills `out` when the push-button extension is enabled. Returns whether a claim was written.
bool pinClaimPushButton(bool enabled, uint8_t pin, ExtPinClaim& out);

// True when two or more enabled claims share a GPIO. Writes a plain message when `message` is set.
bool pinClaimsConflict(
  const ExtPinClaim* claims,
  size_t count,
  char* message = nullptr,
  size_t messageLen = 0
);

// True when `pin` appears in more than one claim (boot idle for contested pins).
bool pinClaimsPinIsContested(const ExtPinClaim* claims, size_t count, uint8_t pin);
