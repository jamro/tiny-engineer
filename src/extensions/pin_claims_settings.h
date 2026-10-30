#pragma once

#include "extensions/pin_claims.h"
#include "settings/values.h"

// Collects claims from every registered extension for the given settings snapshot.
size_t pinClaimsCollect(const SettingsValues& values, ExtPinClaim* out, size_t maxOut);

// True when proposed settings would leave enabled extensions sharing a GPIO.
// Writes a plain conflict message when message/messageLen are set.
bool settingsPinClaimConflict(
  const SettingsValues& proposed,
  char* message = nullptr,
  size_t messageLen = 0
);
