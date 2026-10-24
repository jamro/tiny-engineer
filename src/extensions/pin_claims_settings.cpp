#include "extensions/pin_claims_settings.h"

size_t pinClaimsCollect(const SettingsValues& values, ExtPinClaim* out, size_t maxOut) {
  if (out == nullptr || maxOut == 0) {
    return 0;
  }

  size_t n = 0;

  if (n < maxOut && pinClaimPushButton(values.pushButton, values.pushButtonPin, out[n])) {
    n++;
  }

  return n;
}

bool settingsPinClaimConflict(
  const SettingsValues& proposed,
  char* message,
  size_t messageLen
) {
  ExtPinClaim claims[kExtPinClaimMaxProviders];
  const size_t n = pinClaimsCollect(proposed, claims, kExtPinClaimMaxProviders);
  return pinClaimsConflict(claims, n, message, messageLen);
}
