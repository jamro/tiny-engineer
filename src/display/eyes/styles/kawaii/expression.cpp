#include "display/eyes/styles/kawaii/expression.h"

#include "display/eyes/styles/kawaii/util.h"

#include <cstddef>

namespace kawaii {

namespace {

const char* const kMoodNames[] = {
  "content", "smile", "grin", "wink_left", "wink_right", "excited", "love",
  "focused", "reading", "thinking", "surprised", "curious", "angry", "panic",
  "sleepy", "sad", "dizzy", "talking", "asleep", "pleased", "aha", "suspense", "bliss", "afterglow",
};

static_assert(
  sizeof(kMoodNames) / sizeof(kMoodNames[0]) == (size_t)Mood::Count,
  "every mood needs a name"
);

Expression neutral() {
  Expression e = {};
  e.kind = EyeKind::Round;
  e.pupil = 1.0f;
  e.followGlance = 1.0f;
  e.sparkle = 1.0f;
  e.scale = 1.0f;
  e.mouthWidth = 10.0f;
  e.mouthCurve = 2.0f;
  return e;
}

// "ω" cat mouth.
void catMouth(Expression& e, float width) {
  e.mouthWidth = width;
  e.mouthCurve = 2.8f;
  e.mouthOmega = 1.0f;
}

// Round "o" outline.
void oMouth(Expression& e, float width, float open) {
  e.mouthWidth = width;
  e.mouthCurve = 0.0f;
  e.mouthOpenUp = open;
  e.mouthOpenDown = open;
  e.mouthHollow = 1.0f;
}

void wink(Expression& e, bool left) {
  constexpr float kClosed = 1.0f;
  constexpr float kOther = 0.15f;  // the open eye smiles a little too
  constexpr float kMouthLean = 1.5f;
  e.happyLeft = left ? kClosed : kOther;
  e.happyRight = left ? kOther : kClosed;
  e.mouthShift = left ? -kMouthLean : kMouthLean;
}

}  // namespace

Expression blend(const Expression& a, const Expression& b, float t) {
  t = clamp01(t);
  Expression e = b;
  e.kind = t < 1.0f ? a.kind : b.kind;
  e.happyLeft = lerp(a.happyLeft, b.happyLeft, t);
  e.happyRight = lerp(a.happyRight, b.happyRight, t);
  e.lid = lerp(a.lid, b.lid, t);
  e.tilt = lerp(a.tilt, b.tilt, t);
  e.pupil = lerp(a.pupil, b.pupil, t);
  e.gazeX = lerp(a.gazeX, b.gazeX, t);
  e.gazeY = lerp(a.gazeY, b.gazeY, t);
  e.followGlance = lerp(a.followGlance, b.followGlance, t);
  e.sparkle = lerp(a.sparkle, b.sparkle, t);
  e.scale = lerp(a.scale, b.scale, t);
  e.blush = lerp(a.blush, b.blush, t);
  e.glint = lerp(a.glint, b.glint, t);
  e.bounce = lerp(a.bounce, b.bounce, t);
  e.quiver = lerp(a.quiver, b.quiver, t);
  e.quiverHz = lerp(a.quiverHz, b.quiverHz, t);
  e.mouthWidth = lerp(a.mouthWidth, b.mouthWidth, t);
  e.mouthCurve = lerp(a.mouthCurve, b.mouthCurve, t);
  e.mouthOmega = lerp(a.mouthOmega, b.mouthOmega, t);
  e.mouthOpenUp = lerp(a.mouthOpenUp, b.mouthOpenUp, t);
  e.mouthOpenDown = lerp(a.mouthOpenDown, b.mouthOpenDown, t);
  e.mouthWave = lerp(a.mouthWave, b.mouthWave, t);
  e.mouthShift = lerp(a.mouthShift, b.mouthShift, t);
  e.mouthHollow = lerp(a.mouthHollow, b.mouthHollow, t);
  return e;
}

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
    case Mood::WinkRight:
      wink(e, mood == Mood::WinkLeft);
      e.mouthWidth = 10.0f;
      e.mouthCurve = 2.6f;
      e.mouthOpenDown = 1.5f;
      e.blush = 0.85f;
      e.glint = 0.7f;
      break;
    case Mood::Excited:
      e.scale = 1.12f;
      e.pupil = 1.08f;  // big shiny pupils
      e.sparkle = 1.1f;
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
      e.pupil = 0.9f;
      e.followGlance = 0.0f;  // the code reader leads, not the typing jitter
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
      e.followGlance = 0.5f;  // a calm look aside, not the full swing
      e.mouthWidth = 8.0f;
      e.mouthCurve = -0.5f;
      e.mouthWave = 0.8f;
      e.mouthShift = 4.0f;
      e.blush = 0.2f;
      break;
    case Mood::Surprised:
      e.scale = 1.12f;
      e.pupil = 0.65f;  // shocked: small pupils in wide eyes
      e.sparkle = 1.2f;
      oMouth(e, 7.0f, 3.0f);
      e.blush = 0.3f;
      break;
    case Mood::Curious:
      e.scale = 1.05f;
      e.pupil = 0.85f;
      e.sparkle = 1.1f;
      oMouth(e, 6.0f, 2.0f);
      e.blush = 0.3f;
      break;
    case Mood::Angry:
      e.lid = 0.32f;
      e.tilt = 0.55f;
      e.pupil = 0.75f;
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
      e.pupil = 0.75f;
      e.gazeY = 0.3f;
      e.sparkle = 0.0f;
      oMouth(e, 6.0f, 2.6f);
      e.blush = 0.3f;
      break;
    case Mood::Sad:
      e.lid = 0.15f;
      e.tilt = -0.5f;
      e.pupil = 1.12f;  // big puppy pupils, looking down
      e.gazeY = 0.45f;
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
      // Resting mouth between syllables: a small, slightly parted smile.
      e.mouthWidth = 9.0f;
      e.mouthCurve = 1.5f;
      e.mouthOpenDown = 0.6f;
      e.blush = 0.5f;
      break;
    case Mood::Asleep:
      catMouth(e, 8.0f);
      e.mouthCurve = 1.6f;
      e.blush = 0.4f;
      break;
    case Mood::Pleased:
      e.lid = 0.12f;
      e.happyLeft = e.happyRight = 0.35f;
      e.pupil = 0.95f;
      e.mouthWidth = 11.0f;
      e.mouthCurve = 3.0f;
      e.blush = 0.85f;
      break;
    case Mood::Aha:
      e.scale = 1.1f;
      e.pupil = 0.8f;   // bright, alert pupils
      e.gazeY = -0.4f;  // looking up at the idea
      e.sparkle = 1.15f;
      e.mouthWidth = 10.0f;
      e.mouthCurve = 2.0f;
      e.mouthOpenDown = 3.0f;
      e.blush = 0.6f;
      e.glint = 1.0f;
      e.bounce = 1.2f;
      break;
    case Mood::Suspense:
      e.lid = 0.35f;       // narrowed in focus
      e.pupil = 0.75f;
      e.gazeY = 0.35f;     // eyes on the bell
      e.scale = 0.97f;
      e.mouthWidth = 6.0f;  // held breath
      e.mouthCurve = 0.0f;
      e.blush = 0.6f;
      e.quiver = 0.35f;    // a nervous tremble
      e.quiverHz = 18.0f;
      break;
    case Mood::Bliss:
      // Open, so the shivering pupils show.
      e.happyLeft = e.happyRight = 0.3f;
      e.scale = 1.08f;
      e.pupil = 1.1f;
      e.sparkle = 1.25f;
      e.mouthWidth = 12.0f;
      e.mouthCurve = 2.5f;
      e.mouthOpenDown = 3.5f;
      e.blush = 1.0f;
      e.glint = 1.0f;
      e.bounce = 0.6f;
      e.quiver = 1.0f;
      e.quiverHz = 14.0f;
      break;
    case Mood::Afterglow:
      e.happyLeft = e.happyRight = 0.55f;
      e.lid = 0.2f;
      e.gazeY = -0.25f;  // dreamy, looking up
      catMouth(e, 12.0f);
      e.blush = 0.9f;
      e.glint = 0.4f;
      break;
    case Mood::Count:
      break;
  }

  return e;
}

}  // namespace kawaii
