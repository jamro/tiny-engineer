#include "extensions/pin_claims.h"

#include <cstdio>
#include <cstring>

bool pinClaimPushButton(bool enabled, uint8_t pin, ExtPinClaim& out) {
  if (!enabled) {
    out = {};
    return false;
  }

  out.id = "push-button";
  out.pins[0] = pin;
  out.pinCount = 1;
  return true;
}

namespace {

bool pinInClaim(const ExtPinClaim& claim, uint8_t pin) {
  for (size_t i = 0; i < claim.pinCount; i++) {
    if (claim.pins[i] == pin) {
      return true;
    }
  }
  return false;
}

size_t claimCountForPin(const ExtPinClaim* claims, size_t count, uint8_t pin) {
  size_t n = 0;
  for (size_t i = 0; i < count; i++) {
    if (pinInClaim(claims[i], pin)) {
      n++;
    }
  }
  return n;
}

}  // namespace

bool pinClaimsPinIsContested(const ExtPinClaim* claims, size_t count, uint8_t pin) {
  return claimCountForPin(claims, count, pin) > 1;
}

bool pinClaimsConflict(
  const ExtPinClaim* claims,
  size_t count,
  char* message,
  size_t messageLen
) {
  if (claims == nullptr || count < 2) {
    return false;
  }

  for (size_t i = 0; i < count; i++) {
    for (size_t p = 0; p < claims[i].pinCount; p++) {
      const uint8_t pin = claims[i].pins[p];
      if (claimCountForPin(claims, count, pin) < 2) {
        continue;
      }

      if (message != nullptr && messageLen > 0) {
        // "pin conflict on GP20 (push-button, other)"
        int written = snprintf(message, messageLen, "pin conflict on GP%u (", static_cast<unsigned>(pin));
        if (written < 0) {
          written = 0;
        }

        bool first = true;
        for (size_t c = 0; c < count && static_cast<size_t>(written) + 1 < messageLen; c++) {
          if (!pinInClaim(claims[c], pin) || claims[c].id == nullptr) {
            continue;
          }
          written += snprintf(
            message + written,
            messageLen - static_cast<size_t>(written),
            "%s%s",
            first ? "" : ", ",
            claims[c].id
          );
          first = false;
          if (written < 0) {
            break;
          }
        }

        if (written >= 0 && static_cast<size_t>(written) + 1 < messageLen) {
          message[written++] = ')';
          message[written] = '\0';
        } else if (messageLen > 0) {
          message[messageLen - 1] = '\0';
        }
      }

      return true;
    }
  }

  return false;
}
