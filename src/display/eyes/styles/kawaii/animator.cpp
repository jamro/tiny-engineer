#include "display/eyes/styles/kawaii/animator.h"

#include <cmath>

#include "display/eyes/styles/kawaii/util.h"

namespace kawaii {

namespace {

// Kind swap blink: close, swap the shape while shut, open again.
constexpr float kKindCloseMs = 70.0f;
constexpr float kKindOpenMs = 120.0f;

// Longest step the easing integrates; longer frame gaps are clamped.
constexpr uint32_t kMaxStepMs = 100;

// Underdamped springs: natural frequency and damping ratio (< 1 overshoots).
struct SpringTuning {
  float hz;
  float zeta;
};
constexpr SpringTuning kScaleSpring = {3.5f, 0.45f};      // eye size: a lively pop
constexpr SpringTuning kHappySpring = {3.2f, 0.6f};       // crescents: softer
constexpr SpringTuning kOpenDownSpring = {5.0f, 0.55f};   // mouth: quick, for speech
constexpr float kSpringSubstepS = 0.008f;  // keeps semi-implicit Euler stable
constexpr float kSecondsPerMs = 0.001f;

// Exponential approach time constants, ms.
constexpr float kMouthTau = 70.0f;
constexpr float kLidTau = 90.0f;
constexpr float kSparkleTau = 120.0f;
constexpr float kGlintTau = 160.0f;
constexpr float kBounceTau = 140.0f;
constexpr float kBlushTau = 280.0f;
constexpr float kGazeTau = 150.0f;  // pupils glide rather than jump

// Fraction of the remaining distance an exponential approach covers in dtMs.
float easeWeight(float dtMs, float tauMs) {
  return 1.0f - expf(-dtMs / tauMs);
}

void approach(float& current, float target, float weight) {
  current += (target - current) * weight;
}

// Semi-implicit Euler in small substeps.
void spring(float& x, float& v, float target, float dtMs, const SpringTuning& tuning) {
  const float omega = kTwoPi * tuning.hz;
  float remaining = dtMs * kSecondsPerMs;
  while (remaining > 0.0f) {
    const float h = fminf(remaining, kSpringSubstepS);
    v += (omega * omega * (target - x) - 2.0f * tuning.zeta * omega * v) * h;
    x += v * h;
    remaining -= h;
  }
}

}  // namespace

void Animator::snap(const Expression& target) {
  current_ = target;
  scaleVel_ = happyLeftVel_ = happyRightVel_ = openDownVel_ = 0.0f;
  kindPhase_ = KindPhase::None;
  kindOpen_ = 1.0f;
}

void Animator::update(const Expression& target, uint32_t dtMs) {
  const float dt = (float)(dtMs > kMaxStepMs ? kMaxStepMs : dtMs);
  advanceKindSwap(target.kind, dt);
  easeValues(target, dt);
}

void Animator::advanceKindSwap(EyeKind target, float dtMs) {
  if (target != current_.kind) {
    kindPhase_ = KindPhase::Closing;
  } else if (kindPhase_ == KindPhase::Closing) {
    kindPhase_ = KindPhase::Opening;  // target changed back before the swap
  }

  switch (kindPhase_) {
    case KindPhase::Closing:
      kindOpen_ -= dtMs / kKindCloseMs;
      if (kindOpen_ <= 0.0f) {
        kindOpen_ = 0.0f;
        current_.kind = target;
        kindPhase_ = KindPhase::Opening;
      }
      break;
    case KindPhase::Opening:
      kindOpen_ += dtMs / kKindOpenMs;
      if (kindOpen_ >= 1.0f) {
        kindOpen_ = 1.0f;
        kindPhase_ = KindPhase::None;
      }
      break;
    case KindPhase::None:
      break;
  }
}

// expf is a slow library call on the C3 (no FPU), and the frame time is
// almost always the same 33 ms: recompute the weights only when it changes.
const Animator::EaseWeights& Animator::weightsFor(float dt) {
  if (dt != weights_.dtMs) {
    weights_.dtMs = dt;
    weights_.mouth = easeWeight(dt, kMouthTau);
    weights_.lid = easeWeight(dt, kLidTau);
    weights_.sparkle = easeWeight(dt, kSparkleTau);
    weights_.glint = easeWeight(dt, kGlintTau);
    weights_.bounce = easeWeight(dt, kBounceTau);
    weights_.blush = easeWeight(dt, kBlushTau);
    weights_.gaze = easeWeight(dt, kGazeTau);
  }
  return weights_;
}

void Animator::easeValues(const Expression& t, float dt) {
  Expression& c = current_;

  spring(c.scale, scaleVel_, t.scale, dt, kScaleSpring);
  spring(c.happyLeft, happyLeftVel_, t.happyLeft, dt, kHappySpring);
  spring(c.happyRight, happyRightVel_, t.happyRight, dt, kHappySpring);
  spring(c.mouthOpenDown, openDownVel_, t.mouthOpenDown, dt, kOpenDownSpring);

  const EaseWeights& w = weightsFor(dt);
  approach(c.lid, t.lid, w.lid);
  approach(c.tilt, t.tilt, w.lid);
  approach(c.sparkle, t.sparkle, w.sparkle);
  approach(c.pupil, t.pupil, w.sparkle);
  approach(c.gazeX, t.gazeX, w.gaze);
  approach(c.gazeY, t.gazeY, w.gaze);
  approach(c.followGlance, t.followGlance, w.gaze);
  approach(c.blush, t.blush, w.blush);
  approach(c.glint, t.glint, w.glint);
  approach(c.bounce, t.bounce, w.bounce);
  approach(c.quiver, t.quiver, w.bounce);
  approach(c.mouthWidth, t.mouthWidth, w.mouth);
  approach(c.mouthCurve, t.mouthCurve, w.mouth);
  approach(c.mouthOmega, t.mouthOmega, w.mouth);
  approach(c.mouthOpenUp, t.mouthOpenUp, w.mouth);
  approach(c.mouthWave, t.mouthWave, w.mouth);
  approach(c.mouthShift, t.mouthShift, w.mouth);
  approach(c.mouthHollow, t.mouthHollow, w.mouth);
}

}  // namespace kawaii
