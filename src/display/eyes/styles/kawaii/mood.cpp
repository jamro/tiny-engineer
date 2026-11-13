#include "display/eyes/styles/kawaii/mood.h"

#include <cmath>

namespace kawaii {

namespace {

constexpr float kTwoPi = 6.28318531f;

constexpr uint32_t kKindCloseMs = 70;
constexpr uint32_t kKindOpenMs = 120;
constexpr uint32_t kMaxStepMs = 100;

// Exponential approach time constants, ms.
constexpr float kMouthTau = 70.0f;
constexpr float kLidTau = 90.0f;
constexpr float kSparkleTau = 120.0f;
constexpr float kGlintTau = 160.0f;
constexpr float kBounceTau = 140.0f;
constexpr float kBlushTau = 280.0f;

const char* const kMoodNames[] = {
  "content", "smile", "grin", "wink_left", "wink_right", "excited", "love",
  "focused", "reading", "thinking", "surprised", "curious", "angry", "panic",
  "sleepy", "sad", "dizzy", "talking", "asleep",
};

static_assert(
  sizeof(kMoodNames) / sizeof(kMoodNames[0]) == (size_t)Mood::Count,
  "every mood needs a name"
);

bool reached(uint32_t now, uint32_t at) {
  return (int32_t)(now - at) >= 0;
}

float approach(float current, float target, float dtMs, float tauMs) {
  return current + (target - current) * (1.0f - expf(-dtMs / tauMs));
}

// Semi-implicit Euler in small steps; underdamped for a bit of pop.
void spring(float& x, float& v, float target, float dtMs, float hz, float zeta) {
  const float omega = kTwoPi * hz;
  float remaining = dtMs * 0.001f;
  while (remaining > 0.0f) {
    const float h = remaining > 0.008f ? 0.008f : remaining;
    v += (omega * omega * (target - x) - 2.0f * zeta * omega * v) * h;
    x += v * h;
    remaining -= h;
  }
}

// -1..1 sine with the given period; modulo keeps float precision for large `now`.
float wave(uint32_t now, uint32_t periodMs) {
  return sinf(kTwoPi * (float)(now % periodMs) / (float)periodMs);
}

Expression neutral() {
  Expression e = {};
  e.kind = EyeKind::Round;
  e.sparkle = 1.0f;
  e.scale = 1.0f;
  e.mouthWidth = 10.0f;
  e.mouthCurve = 2.0f;
  return e;
}

void catMouth(Expression& e, float width) {
  e.mouthWidth = width;
  e.mouthCurve = 2.8f;
  e.mouthOmega = 1.0f;
}

void oMouth(Expression& e, float width, float open) {
  e.mouthWidth = width;
  e.mouthCurve = 0.0f;
  e.mouthOpenUp = open;
  e.mouthOpenDown = open;
  e.mouthHollow = 1.0f;
}

EyePose withHop(EyePose eye, float hop) {
  eye.cy += hop;
  return eye;
}

EyeShape eyeFrom(
  const EyePose& pose,
  int8_t side,
  const Expression& e,
  float happy,
  float open,
  float sparkle
) {
  EyeShape eye = {};
  eye.kind = e.kind;
  eye.side = side;
  eye.cx = pose.cx;
  eye.cy = pose.cy;
  eye.rx = kEyeRadius * e.scale;
  eye.ry = eye.rx * pose.heightScale;
  eye.open = open;
  eye.happy = happy;
  eye.lid = e.lid;
  eye.tilt = e.tilt;
  eye.sparkle = sparkle;
  return eye;
}

}  // namespace

const char* moodName(Mood mood) {
  const size_t index = (size_t)mood;
  if (index >= (size_t)Mood::Count) {
    return nullptr;
  }
  return kMoodNames[index];
}

Expression expressionFor(Mood mood) {
  Expression e = neutral();

  switch (mood) {
    case Mood::Content:
      catMouth(e, 13.0f);
      e.blush = 0.75f;
      break;
    case Mood::Smile:
      e.happyLeft = e.happyRight = 0.22f;
      e.mouthWidth = 12.0f;
      e.mouthCurve = 3.0f;
      e.blush = 0.8f;
      break;
    case Mood::Grin:
      e.happyLeft = e.happyRight = 1.0f;
      e.mouthWidth = 12.0f;
      e.mouthCurve = 2.0f;
      e.mouthOpenDown = 3.5f;
      e.blush = 0.9f;
      break;
    case Mood::WinkLeft:
    case Mood::WinkRight: {
      const bool left = mood == Mood::WinkLeft;
      e.happyLeft = left ? 1.0f : 0.15f;
      e.happyRight = left ? 0.15f : 1.0f;
      e.mouthWidth = 10.0f;
      e.mouthCurve = 2.6f;
      e.mouthOpenDown = 1.5f;
      e.mouthShift = left ? -1.5f : 1.5f;
      e.blush = 0.85f;
      e.glint = 0.7f;
      break;
    }
    case Mood::Excited:
      e.scale = 1.12f;
      e.sparkle = 1.25f;
      e.mouthWidth = 12.0f;
      e.mouthCurve = 2.0f;
      e.mouthOpenDown = 4.5f;
      e.blush = 1.0f;
      e.glint = 1.0f;
      e.bounce = 1.8f;
      break;
    case Mood::Love:
      e.kind = EyeKind::Heart;
      catMouth(e, 13.0f);
      e.blush = 1.0f;
      e.glint = 0.6f;
      break;
    case Mood::Focused:
      e.lid = 0.22f;
      e.mouthWidth = 7.0f;
      e.mouthCurve = 0.5f;
      e.blush = 0.2f;
      break;
    case Mood::Reading:
      e.lid = 0.12f;
      catMouth(e, 9.0f);
      e.mouthCurve = 1.8f;
      e.blush = 0.35f;
      break;
    case Mood::Thinking:
      e.lid = 0.15f;
      e.mouthWidth = 8.0f;
      e.mouthCurve = -0.5f;
      e.mouthWave = 0.8f;
      e.mouthShift = 4.0f;
      e.blush = 0.2f;
      break;
    case Mood::Surprised:
      e.scale = 1.12f;
      e.sparkle = 1.2f;
      oMouth(e, 7.0f, 3.0f);
      e.blush = 0.3f;
      break;
    case Mood::Curious:
      e.scale = 1.05f;
      e.sparkle = 1.1f;
      oMouth(e, 6.0f, 2.0f);
      e.blush = 0.3f;
      break;
    case Mood::Angry:
      e.lid = 0.32f;
      e.tilt = 0.55f;
      e.sparkle = 0.8f;
      e.mouthWidth = 9.0f;
      e.mouthCurve = -2.2f;
      break;
    case Mood::Panic:
      e.kind = EyeKind::Chevron;
      e.mouthWidth = 14.0f;
      e.mouthCurve = 0.0f;
      e.mouthWave = 1.3f;
      e.blush = 0.4f;
      break;
    case Mood::Sleepy:
      e.lid = 0.45f;
      e.sparkle = 0.0f;
      oMouth(e, 6.0f, 2.6f);
      e.blush = 0.3f;
      break;
    case Mood::Sad:
      e.lid = 0.15f;
      e.tilt = -0.5f;
      e.sparkle = 1.15f;
      e.mouthWidth = 8.0f;
      e.mouthCurve = -2.0f;
      break;
    case Mood::Dizzy:
      e.kind = EyeKind::Cross;
      e.mouthWidth = 12.0f;
      e.mouthCurve = -1.0f;
      e.mouthWave = 1.0f;
      break;
    case Mood::Talking:
      e.mouthWidth = 9.0f;
      e.mouthCurve = 1.5f;
      e.blush = 0.5f;
      break;
    case Mood::Asleep:
      catMouth(e, 8.0f);
      e.mouthCurve = 1.6f;
      e.blush = 0.4f;
      break;
    case Mood::Count:
      break;
  }

  return e;
}

void applySpeech(Expression& target, float level, bool speaking) {
  if (level < 0.0f) {
    level = 0.0f;
  } else if (level > 1.0f) {
    level = 1.0f;
  }

  if (!speaking) {
    level *= 0.3f;
  }

  target.mouthOpenUp = level * 1.5f;
  target.mouthOpenDown = 0.6f + level * 4.5f;
  target.mouthWidth = 8.0f + level * 3.0f;
}

void Animator::snap(const Expression& target) {
  current_ = target;
  scaleVel_ = happyLeftVel_ = happyRightVel_ = openDownVel_ = 0.0f;
  kindPhase_ = KindPhase::None;
  kindOpen_ = 1.0f;
}

void Animator::update(const Expression& target, uint32_t dtMs) {
  if (dtMs > kMaxStepMs) {
    dtMs = kMaxStepMs;
  }
  const float dt = (float)dtMs;

  // Kind swap: close, swap while shut, open again.
  if (target.kind != current_.kind) {
    kindPhase_ = KindPhase::Closing;
  } else if (kindPhase_ == KindPhase::Closing) {
    kindPhase_ = KindPhase::Opening;
  }

  if (kindPhase_ == KindPhase::Closing) {
    kindOpen_ -= dt / kKindCloseMs;
    if (kindOpen_ <= 0.0f) {
      kindOpen_ = 0.0f;
      current_.kind = target.kind;
      kindPhase_ = KindPhase::Opening;
    }
  } else if (kindPhase_ == KindPhase::Opening) {
    kindOpen_ += dt / kKindOpenMs;
    if (kindOpen_ >= 1.0f) {
      kindOpen_ = 1.0f;
      kindPhase_ = KindPhase::None;
    }
  }

  spring(current_.scale, scaleVel_, target.scale, dt, 3.5f, 0.45f);
  spring(current_.happyLeft, happyLeftVel_, target.happyLeft, dt, 3.2f, 0.6f);
  spring(current_.happyRight, happyRightVel_, target.happyRight, dt, 3.2f, 0.6f);
  spring(current_.mouthOpenDown, openDownVel_, target.mouthOpenDown, dt, 5.0f, 0.55f);

  current_.lid = approach(current_.lid, target.lid, dt, kLidTau);
  current_.tilt = approach(current_.tilt, target.tilt, dt, kLidTau);
  current_.sparkle = approach(current_.sparkle, target.sparkle, dt, kSparkleTau);
  current_.blush = approach(current_.blush, target.blush, dt, kBlushTau);
  current_.glint = approach(current_.glint, target.glint, dt, kGlintTau);
  current_.bounce = approach(current_.bounce, target.bounce, dt, kBounceTau);
  current_.mouthWidth = approach(current_.mouthWidth, target.mouthWidth, dt, kMouthTau);
  current_.mouthCurve = approach(current_.mouthCurve, target.mouthCurve, dt, kMouthTau);
  current_.mouthOmega = approach(current_.mouthOmega, target.mouthOmega, dt, kMouthTau);
  current_.mouthOpenUp = approach(current_.mouthOpenUp, target.mouthOpenUp, dt, kMouthTau);
  current_.mouthWave = approach(current_.mouthWave, target.mouthWave, dt, kMouthTau);
  current_.mouthShift = approach(current_.mouthShift, target.mouthShift, dt, kMouthTau);
  current_.mouthHollow = approach(current_.mouthHollow, target.mouthHollow, dt, kMouthTau);
}

void IdleDirector::reset(uint32_t now) {
  armed_ = true;
  base_ = Mood::Smile;
  accentActive_ = false;
  baseSwapAt_ = now + rand_(3000, 8000);
  accentAt_ = now + rand_(3500, 8000);
}

Mood IdleDirector::pickAccent(uint32_t now) {
  const uint32_t roll = rand_(0, 100);
  uint32_t duration = 0;

  if (roll < 34) {
    accent_ = Mood::Grin;
    duration = rand_(1400, 2600);
  } else if (roll < 64) {
    accent_ = rand_(0, 2) == 0 ? Mood::WinkLeft : Mood::WinkRight;
    duration = rand_(550, 900);
  } else if (roll < 90) {
    accent_ = Mood::Excited;
    duration = rand_(1200, 1900);
  } else {
    accent_ = Mood::Love;
    duration = rand_(1500, 2200);
  }

  accentActive_ = true;
  accentEndAt_ = now + duration;
  return accent_;
}

Mood IdleDirector::update(uint32_t now) {
  if (!armed_) {
    reset(now);
  }

  if (accentActive_) {
    if (!reached(now, accentEndAt_)) {
      return accent_;
    }
    accentActive_ = false;
    accentAt_ = now + rand_(4500, 10000);
    // Settle on a gentle smile after an accent.
    base_ = Mood::Smile;
    baseSwapAt_ = now + rand_(2500, 6000);
  }

  if (reached(now, accentAt_)) {
    return pickAccent(now);
  }

  if (reached(now, baseSwapAt_)) {
    base_ = base_ == Mood::Content ? Mood::Smile : Mood::Content;
    baseSwapAt_ = now + rand_(3000, 8000);
  }

  return base_;
}

void composeFace(const Pose& pose, const Animator& animator, uint32_t now, Face& out) {
  const Expression& e = animator.current();

  // Hop: bounces off the floor rather than swinging through it.
  const float hop = -fabsf(wave(now, 360)) * e.bounce;
  const float twinkle = 1.0f + 0.2f * e.glint * wave(now, 480);
  const float open = pose.open * animator.kindOpen();

  const EyePose left = withHop(pose.left, hop);
  const EyePose right = withHop(pose.right, hop);

  out.left = eyeFrom(left, 1, e, e.happyLeft, open, e.sparkle * twinkle);
  out.right = eyeFrom(right, -1, e, e.happyRight, open, e.sparkle * twinkle);

  out.mouth.cx = (left.cx + right.cx) * 0.5f + e.mouthShift;
  out.mouth.cy = (left.cy + right.cy) * 0.5f + kMouthDrop;
  out.mouth.width = e.mouthWidth;
  out.mouth.curve = e.mouthCurve;
  out.mouth.omega = e.mouthOmega;
  out.mouth.openUp = e.mouthOpenUp;
  out.mouth.openDown = e.mouthOpenDown;
  out.mouth.wave = e.mouthWave;
  out.mouth.hollow = e.mouthHollow >= 0.5f;

  out.blush = e.blush;
  out.glint = e.glint * (1.6f + 1.4f * wave(now, 560));
}

}  // namespace kawaii
