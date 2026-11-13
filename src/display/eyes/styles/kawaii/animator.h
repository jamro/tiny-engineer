#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/expression.h"

namespace kawaii {

// Eases the current expression toward a target. Springs on eye size, happy
// crescents and mouth opening give a little overshoot; shape swaps that
// cannot morph (round → > <, hearts, x) happen behind a quick blink.
class Animator {
 public:
  // Jump straight to `target` with no motion in flight.
  void snap(const Expression& target);
  // Advance by dtMs (clamped, so a stall does not fling the springs).
  void update(const Expression& target, uint32_t dtMs);

  const Expression& current() const { return current_; }
  // 0..1 extra eye openness for the blink that hides a kind swap.
  float kindOpen() const { return kindOpen_; }

 private:
  enum class KindPhase : uint8_t { None, Closing, Opening };

  // Per-group easing weights (1 - e^(-dt/tau)) for the last frame time.
  struct EaseWeights {
    float dtMs = -1.0f;  // none computed yet
    float mouth = 0.0f;
    float lid = 0.0f;
    float sparkle = 0.0f;
    float glint = 0.0f;
    float bounce = 0.0f;
    float blush = 0.0f;
    float gaze = 0.0f;
  };

  void advanceKindSwap(EyeKind target, float dtMs);
  void easeValues(const Expression& target, float dtMs);
  const EaseWeights& weightsFor(float dtMs);

  Expression current_ = {};
  float scaleVel_ = 0.0f;
  float happyLeftVel_ = 0.0f;
  float happyRightVel_ = 0.0f;
  float openDownVel_ = 0.0f;
  KindPhase kindPhase_ = KindPhase::None;
  float kindOpen_ = 1.0f;
  EaseWeights weights_;
};

}  // namespace kawaii
