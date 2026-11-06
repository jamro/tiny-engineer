#include "extensions/push_button_logic.h"

namespace {

void resetPhase(PushButtonLogic& logic) {
  logic.phase = PushButtonPhase::Idle;
  logic.downUs = 0;
  logic.shortUpUs = 0;
}

}  // namespace

void PushButtonLogic::apply(bool on, PushButtonSample sample) {
  if (!on) {
    enabled = false;
    lastAcceptedHigh = true;
    lastAcceptedUs = 0;
    resetPhase(*this);
    pendingGesture.reset();
    immediateGesture.reset();
    return;
  }

  enabled = true;
  // Always start as "released". Enable-while-held then stays silent until the
  // next press (release after enable does not publish).
  lastAcceptedHigh = true;
  lastAcceptedUs = sample.us;
  resetPhase(*this);
  pendingGesture.reset();
  immediateGesture.reset();
}

std::optional<PushButtonEvent> PushButtonLogic::takeImmediateGesture() {
  if (!immediateGesture.has_value()) {
    return std::nullopt;
  }
  const auto gesture = *immediateGesture;
  immediateGesture.reset();
  return gesture;
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
  if (!sample.levelHigh) {
    if (phase == PushButtonPhase::WaitSecond) {
      if (sample.us - shortUpUs < kPushButtonDoubleWaitUs) {
        phase = PushButtonPhase::HoldingSecond;
        downUs = sample.us;
      } else {
        // Window expired but tick has not run yet — click before this down.
        immediateGesture = PushButtonEvent::Click;
        phase = PushButtonPhase::Holding;
        downUs = sample.us;
        shortUpUs = 0;
      }
    } else {
      phase = PushButtonPhase::Holding;
      downUs = sample.us;
    }
    return PushButtonEvent::Down;
  }

  // Up
  const bool wasSecond = phase == PushButtonPhase::HoldingSecond;
  const bool wasLong = phase == PushButtonPhase::LongFired;
  const uint64_t holdUs = sample.us >= downUs ? sample.us - downUs : 0;

  if (wasLong) {
    resetPhase(*this);
    return PushButtonEvent::Up;
  }

  if (wasSecond) {
    if (holdUs < kPushButtonLongHoldUs) {
      pendingGesture = PushButtonEvent::DoubleClick;
    }
    resetPhase(*this);
    return PushButtonEvent::Up;
  }

  if (phase == PushButtonPhase::Holding && holdUs < kPushButtonLongHoldUs) {
    phase = PushButtonPhase::WaitSecond;
    shortUpUs = sample.us;
    return PushButtonEvent::Up;
  }

  // Holding past long without a tick: treat like long path (up only).
  resetPhase(*this);
  return PushButtonEvent::Up;
}

std::optional<PushButtonEvent> PushButtonLogic::tick(uint64_t nowUs) {
  if (!enabled) {
    return std::nullopt;
  }

  if (pendingGesture.has_value()) {
    const auto gesture = *pendingGesture;
    pendingGesture.reset();
    return gesture;
  }

  if (phase == PushButtonPhase::Holding || phase == PushButtonPhase::HoldingSecond) {
    if (nowUs >= downUs && nowUs - downUs >= kPushButtonLongHoldUs) {
      phase = PushButtonPhase::LongFired;
      return PushButtonEvent::LongClick;
    }
  }

  if (phase == PushButtonPhase::WaitSecond) {
    if (nowUs >= shortUpUs && nowUs - shortUpUs >= kPushButtonDoubleWaitUs) {
      phase = PushButtonPhase::Idle;
      shortUpUs = 0;
      return PushButtonEvent::Click;
    }
  }

  return std::nullopt;
}
