#include <Arduino.h>

#include "animation.h"
#include "animation/constants.h"
#include "animation/typing.h"
#include "animation/util.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"
#include "serial_log.h"

using anim::parkTorso;
using anim::randChance;
using anim::randRangeMs;
using anim::randUnit;
using anim::snapHeadToRangeHigh;
using anim::stopAnimServos;

namespace {

bool g_typingMoveRight = true;
bool g_headHigh = true;
bool g_bodySwayPositive = true;
bool g_swayFrozen = false;
uint32_t g_handPauseUntilMs = 0;
uint32_t g_headPauseUntilMs = 0;
uint32_t g_swayPauseUntilMs = 0;

void commandBodyNeckSway() {
  const float offset =
    g_bodySwayPositive ? anim::TYPING_SWAY_DEG : -anim::TYPING_SWAY_DEG;
  const float speedDegS =
    12.0f + 18.0f * randUnit();

  servoAt(SERVO_BODY).setTarget(
    anim::TYPING_BODY_MID + offset,
    speedDegS
  );
  servoAt(SERVO_NECK).setTarget(
    anim::TYPING_NECK_MID - offset,
    speedDegS
  );

  serialLogPrint("[anim] typing sway body->");
  serialLogPrint(anim::TYPING_BODY_MID + offset, 1);
  serialLogPrint(" neck->");
  serialLogPrint(anim::TYPING_NECK_MID - offset, 1);
  serialLogPrint(" speed=");
  serialLogPrint(speedDegS, 1);
  serialLogPrint(" dir=");
  serialLogPrintln(g_bodySwayPositive ? "right" : "left");
}

void commandHandStroke() {
  if (!randChance(45)) {
    g_typingMoveRight = !g_typingMoveRight;
  }

  const float depth = 0.35f + 0.45f * randUnit();
  const bool pressHigh = randChance(50);
  const float speedDegS =
    SERVO_MAX_SPEED_DEG_S * (0.75f + 0.25f * randUnit());

  if (g_typingMoveRight) {
    const float target = pressHigh
      ? anim::TYPING_RIGHT_LOW + depth * anim::TYPING_HAND_BAND_DEG
      : anim::TYPING_RIGHT_HIGH - depth * anim::TYPING_HAND_BAND_DEG;
    servoAt(SERVO_HAND_RIGHT).setTarget(target, speedDegS);
  } else {
    const float target = pressHigh
      ? anim::TYPING_LEFT_LOW + depth * anim::TYPING_HAND_BAND_DEG
      : anim::TYPING_LEFT_HIGH - depth * anim::TYPING_HAND_BAND_DEG;
    servoAt(SERVO_HAND_LEFT).setTarget(target, speedDegS);
  }
}

void commandHead() {
  if (g_headHigh) {
    const float upSpeed =
      35.0f + 25.0f * randUnit();
    servoAt(SERVO_HEAD).setTarget(
      anim::TYPING_HEAD_HIGH,
      upSpeed
    );
  } else {
    const float downSpeed =
      6.0f + 6.0f * randUnit();
    servoAt(SERVO_HEAD).setTarget(
      anim::TYPING_HEAD_LOW,
      downSpeed
    );
  }
}

void advanceTypingStep() {
  const uint32_t pauseMs = randChance(8)
    ? randRangeMs(40, 90)
    : randRangeMs(0, 25);
  g_handPauseUntilMs = millis() + pauseMs;
}

void beginNextHandStroke() {
  g_handPauseUntilMs = 0;
  commandHandStroke();
}

void advanceHeadStep() {
  g_headHigh = !g_headHigh;

  if (g_headHigh) {
    g_headPauseUntilMs = millis() + randRangeMs(80, 220);
  } else {
    g_headPauseUntilMs = millis() + randRangeMs(120, 350);
  }
}

void beginNextHeadMove() {
  g_headPauseUntilMs = 0;
  commandHead();
}

void advanceSwayStep() {
  g_bodySwayPositive = !g_bodySwayPositive;
  g_swayPauseUntilMs = millis() + randRangeMs(40, 160);
}

void beginNextSway() {
  g_swayPauseUntilMs = 0;
  commandBodyNeckSway();
}

}  // namespace

void startTyping() {
  stopAnimServos();
  g_typingMoveRight = randChance(50);
  g_bodySwayPositive = randChance(50);
  g_swayFrozen = false;
  g_handPauseUntilMs = 0;
  g_swayPauseUntilMs = 0;
  g_headHigh = true;
  g_headPauseUntilMs = 0;
  commandBodyNeckSway();
  commandHandStroke();
  snapHeadToRangeHigh(anim::TYPING_HEAD_HIGH);
}

void updateTyping(uint32_t now) {
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

  if (hasPendingAnimation() && !g_swayFrozen) {
    g_swayFrozen = true;
    g_swayPauseUntilMs = 0;
    parkTorso(anim::TRANSITION_TORSO_SPEED_DEG_S);
  }

  if (g_handPauseUntilMs != 0) {
    if (now >= g_handPauseUntilMs) {
      beginNextHandStroke();
    }
  } else {
    const bool activeDone = g_typingMoveRight
      ? !right.isMoving()
      : !left.isMoving();

    if (activeDone) {
      advanceTypingStep();
    }
  }

  if (g_headPauseUntilMs != 0) {
    if (now >= g_headPauseUntilMs) {
      beginNextHeadMove();
    }
  } else if (!head.isMoving()) {
    advanceHeadStep();
  }

  if (!g_swayFrozen) {
    if (g_swayPauseUntilMs != 0) {
      if (now >= g_swayPauseUntilMs) {
        beginNextSway();
      }
    } else if (!body.isMoving() && !neck.isMoving()) {
      advanceSwayStep();
    }
  }
}
