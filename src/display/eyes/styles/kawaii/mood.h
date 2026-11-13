#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/face.h"

// Moods, easing, and the idle personality for the kawaii eye style.
// Pure C++ like face.h: the firmware glue lives in styles/kawaii.cpp.

namespace kawaii {

enum class Mood : uint8_t {
  // Idle family: mostly smiles, with the occasional accent.
  Content,    // ω cat mouth
  Smile,      // ‿
  Grin,       // ^ ^ happy crescents, open smile
  WinkLeft,
  WinkRight,
  Excited,    // bigger eyes, hop, twinkles, open grin
  Love,       // heart eyes
  // Mode moods.
  Focused,    // typing
  Reading,
  Thinking,
  Surprised,  // ring
  Curious,    // attention
  Angry,      // error
  Panic,      // abort: > <
  Sleepy,     // wakeup yawn
  Sad,        // dead, before the X
  Dizzy,      // dead: x x
  Talking,    // mouth follows speech loudness
  Asleep,
  Count
};

const char* moodName(Mood mood);

// Every value the animator eases. Geometry (where the eyes are, how tall,
// blinks) comes from the shared pose and is not part of the expression.
struct Expression {
  EyeKind kind;
  float happyLeft;
  float happyRight;
  float lid;
  float tilt;
  float sparkle;
  float scale;          // eye radius multiplier
  float blush;
  float glint;
  float bounce;         // hop height, px
  float mouthWidth;
  float mouthCurve;
  float mouthOmega;
  float mouthOpenUp;
  float mouthOpenDown;
  float mouthWave;
  float mouthShift;     // px sideways
  float mouthHollow;    // 0 filled .. 1 outline
};

Expression expressionFor(Mood mood);

// Talking: opens the mouth with speech loudness (0..1).
void applySpeech(Expression& target, float level, bool speaking);

// Eases the current expression toward a target. Springs on eye size, happy
// crescents and mouth opening give a little overshoot; shape swaps that
// cannot morph (round → > <, hearts, x) happen behind a quick blink.
class Animator {
 public:
  void snap(const Expression& target);
  void update(const Expression& target, uint32_t dtMs);

  const Expression& current() const { return current_; }
  // 0..1 extra eye openness for the blink that hides a kind swap.
  float kindOpen() const { return kindOpen_; }

 private:
  enum class KindPhase : uint8_t { None, Closing, Opening };

  Expression current_ = {};
  float scaleVel_ = 0.0f;
  float happyLeftVel_ = 0.0f;
  float happyRightVel_ = 0.0f;
  float openDownVel_ = 0.0f;
  KindPhase kindPhase_ = KindPhase::None;
  float kindOpen_ = 1.0f;
};

// Idle personality: alternates Content and Smile, and every few seconds
// plays a short accent (grin, wink, excited, or rarely heart eyes).
class IdleDirector {
 public:
  // Returns a value in [lo, hi). Firmware passes anim::randRangeMs.
  using RandRange = uint32_t (*)(uint32_t lo, uint32_t hi);

  explicit IdleDirector(RandRange rand) : rand_(rand) {}

  void reset(uint32_t now);
  Mood update(uint32_t now);

 private:
  Mood pickAccent(uint32_t now);

  RandRange rand_;
  bool armed_ = false;
  Mood base_ = Mood::Smile;
  bool accentActive_ = false;
  Mood accent_ = Mood::Grin;
  uint32_t baseSwapAt_ = 0;
  uint32_t accentAt_ = 0;
  uint32_t accentEndAt_ = 0;
};

struct EyePose {
  float cx;
  float cy;
  float heightScale;  // pose height / classic open height
};

struct Pose {
  EyePose left;
  EyePose right;
  float open;  // blink / sleep lid, 0..1
};

constexpr float kEyeRadius = 11.0f;
constexpr float kMouthDrop = 7.5f;

// Builds the face to render from the pose, the eased expression, and time
// (hop, twinkle).
void composeFace(const Pose& pose, const Animator& animator, uint32_t now, Face& out);

}  // namespace kawaii
