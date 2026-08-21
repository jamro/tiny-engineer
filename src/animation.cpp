#include <Arduino.h>
#include <string.h>

#include "animation.h"
#include "servos.h"
#include "servo_wrapper.h"

namespace {

AnimationId g_animation = AnimationId::None;

// Right/left scales inverted: same numeric direction = opposite physical.
// Alternate hands (one moves at a time) like typing.
// Right: first 10° of scale; left: last 10° of scale.
// Head: lowest 10° of scale; slow down, fast up (follow text).
bool g_typingMoveRight = true;
bool g_rightHigh = false;
bool g_leftHigh = false;
bool g_headHigh = true;

constexpr float TYPING_BAND_DEG = 10.0f;
constexpr float TYPING_HEAD_DOWN_DEG_S = 12.0f;

constexpr float TYPING_RIGHT_LOW =
  SERVO_SPECS[SERVO_HAND_RIGHT].min;
constexpr float TYPING_RIGHT_HIGH =
  SERVO_SPECS[SERVO_HAND_RIGHT].min + TYPING_BAND_DEG;
constexpr float TYPING_LEFT_LOW =
  SERVO_SPECS[SERVO_HAND_LEFT].max - TYPING_BAND_DEG;
constexpr float TYPING_LEFT_HIGH =
  SERVO_SPECS[SERVO_HAND_LEFT].max;

constexpr float TYPING_HEAD_LOW =
  SERVO_SPECS[SERVO_HEAD].min;
constexpr float TYPING_HEAD_HIGH =
  SERVO_SPECS[SERVO_HEAD].min + TYPING_BAND_DEG;

void stopTypingServos() {
  servoAt(SERVO_HAND_LEFT).stop();
  servoAt(SERVO_HAND_RIGHT).stop();
  servoAt(SERVO_HEAD).stop();
  servoAt(SERVO_NECK).stop();
  servoAt(SERVO_BODY).stop();
}

void parkAllServosMid() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    servoAt(i).setTarget(servoMid(SERVO_SPECS[i]));
  }
}

void parkNeckAndBody() {
  servoAt(SERVO_NECK).setTarget(
    servoMid(SERVO_SPECS[SERVO_NECK])
  );
  servoAt(SERVO_BODY).setTarget(
    servoMid(SERVO_SPECS[SERVO_BODY])
  );
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

void commandHead() {
  if (g_headHigh) {
    servoAt(SERVO_HEAD).setTarget(
      TYPING_HEAD_HIGH,
      SERVO_MAX_SPEED_DEG_S
    );
  } else {
    servoAt(SERVO_HEAD).setTarget(
      TYPING_HEAD_LOW,
      TYPING_HEAD_DOWN_DEG_S
    );
  }
}

void startTyping() {
  stopTypingServos();
  g_typingMoveRight = true;
  g_rightHigh = true;
  g_leftHigh = true;
  g_headHigh = false;
  parkNeckAndBody();
  commandActiveHand();
  commandHead();
}

void startNone() {
  parkAllServosMid();
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

void advanceHeadStep() {
  g_headHigh = !g_headHigh;
  commandHead();
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
  if (g_animation == AnimationId::None) {
    updateAllServos();
    return;
  }

  if (g_animation != AnimationId::Typing) {
    return;
  }

  ServoWrapper& left = servoAt(SERVO_HAND_LEFT);
  ServoWrapper& right = servoAt(SERVO_HAND_RIGHT);
  ServoWrapper& head = servoAt(SERVO_HEAD);
  ServoWrapper& neck = servoAt(SERVO_NECK);
  ServoWrapper& body = servoAt(SERVO_BODY);

  left.update();
  right.update();
  head.update();
  neck.update();
  body.update();

  const bool activeDone = g_typingMoveRight
    ? !right.isMoving()
    : !left.isMoving();

  if (activeDone) {
    advanceTypingStep();
  }

  if (!head.isMoving()) {
    advanceHeadStep();
  }
}
