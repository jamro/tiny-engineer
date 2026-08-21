#include <Arduino.h>
#include <string.h>

#include "animation.h"
#include "servos.h"
#include "servo_wrapper.h"

namespace {

AnimationId g_animation = AnimationId::None;

// Right/left scales inverted: same numeric direction = opposite physical.
// Alternate hands (one moves at a time) like typing.
bool g_typingMoveRight = true;
bool g_rightHigh = false;
bool g_leftHigh = false;

constexpr float TYPING_RIGHT_LOW = 40.0f;
constexpr float TYPING_RIGHT_HIGH = 50.0f;
constexpr float TYPING_LEFT_LOW = 130.0f;
constexpr float TYPING_LEFT_HIGH = 140.0f;

void stopHands() {
  servoAt(SERVO_HAND_LEFT).stop();
  servoAt(SERVO_HAND_RIGHT).stop();
}

void commandActiveHand() {
  if (g_typingMoveRight) {
    const float target =
      g_rightHigh ? TYPING_RIGHT_HIGH : TYPING_RIGHT_LOW;
    servoAt(SERVO_HAND_RIGHT).setTarget(target);
  } else {
    const float target =
      g_leftHigh ? TYPING_LEFT_HIGH : TYPING_LEFT_LOW;
    servoAt(SERVO_HAND_LEFT).setTarget(target);
  }
}

void startTyping() {
  stopHands();
  g_typingMoveRight = true;
  g_rightHigh = true;
  g_leftHigh = true;
  commandActiveHand();
}

void startNone() {
  stopHands();
}

void advanceTypingStep() {
  if (g_typingMoveRight) {
    g_rightHigh = !g_rightHigh;
  } else {
    g_leftHigh = !g_leftHigh;
  }

  g_typingMoveRight = !g_typingMoveRight;
  commandActiveHand();
}

}  // namespace

void setAnimation(AnimationId id) {
  g_animation = id;

  switch (id) {
    case AnimationId::Typing:
      startTyping();
      break;
    case AnimationId::None:
    default:
      startNone();
      break;
  }
}

AnimationId getAnimation() {
  return g_animation;
}

const char* animationName(AnimationId id) {
  switch (id) {
    case AnimationId::Typing:
      return "typing";
    case AnimationId::None:
    default:
      return "none";
  }
}

bool parseAnimationName(const char* name, AnimationId& out) {
  if (name == nullptr) {
    return false;
  }

  if (strcmp(name, "none") == 0) {
    out = AnimationId::None;
    return true;
  }

  if (strcmp(name, "typing") == 0) {
    out = AnimationId::Typing;
    return true;
  }

  return false;
}

void updateAnimation() {
  if (g_animation != AnimationId::Typing) {
    return;
  }

  ServoWrapper& left = servoAt(SERVO_HAND_LEFT);
  ServoWrapper& right = servoAt(SERVO_HAND_RIGHT);

  left.update();
  right.update();

  const bool activeDone = g_typingMoveRight
    ? !right.isMoving()
    : !left.isMoving();

  if (activeDone) {
    advanceTypingStep();
  }
}
