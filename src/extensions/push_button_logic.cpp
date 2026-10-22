#include "extensions/push_button_logic.h"

void PushButtonLogic::apply(bool on, PushButtonSample sample) {
  if (!on) {
    enabled = false;
    lastAcceptedHigh = true;
    lastAcceptedUs = 0;
    return;
  }

  enabled = true;
  // Always start as "released". Enable-while-held then stays silent until the
  // next press (release after enable does not publish).
  lastAcceptedHigh = true;
  lastAcceptedUs = sample.us;
}

std::optional<PushButtonEvent> PushButtonLogic::accept(PushButtonSample sample) {
  if (!enabled) {
    return std::nullopt;
  }

  if (sample.levelHigh == lastAcceptedHigh) {
    return std::nullopt;
  }

  if (sample.us < lastAcceptedUs) {
    return std::nullopt;
  }

  if (sample.us - lastAcceptedUs < kPushButtonDebounceUs) {
    return std::nullopt;
  }

  lastAcceptedHigh = sample.levelHigh;
  lastAcceptedUs = sample.us;

  // Active-low: LOW → pressed (down), HIGH → released (up).
  return sample.levelHigh ? PushButtonEvent::Up : PushButtonEvent::Down;
}
