#include <Arduino.h>

#include "animation.h"
#include "animation/constants.h"
#include "animation/talking.h"
#include "animation/util.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"

using anim::randRangeMs;
using anim::randUnit;
using anim::stopAnimServos;

namespace {

bool g_poseFrozen = false;
bool g_headUp = false;
bool g_gestureRaised = false;
uint32_t g_headPauseUntilMs = 0;
uint32_t g_neckNextMs = 0;
uint32_t g_gestureNextMs = 0;

void commandHeadBeat() {
  // Quick up on the beat, slower settle, so it reads as emphasis rather than a bob.
  g_headUp = !g_headUp;
  const float target = g_headUp
    ? anim::TALKING_HEAD_MID + anim::TALKING_HEAD_NOD * (0.4f + 0.6f * randUnit())
    : anim::TALKING_HEAD_MID;
  const float speed = g_headUp ? 45.0f + 30.0f * randUnit() : 20.0f + 15.0f * randUnit();
  servoAt(SERVO_HEAD).setNormTarget(target, speed);
}

void commandNeckDrift() {
  const float target = anim::TALKING_NECK_SWAY * (2.0f * randUnit() - 1.0f);
  servoAt(SERVO_NECK).setNormTarget(target, 10.0f + 10.0f * randUnit());
}

void commandGesture() {
  g_gestureRaised = !g_gestureRaised;
  const float lift = g_gestureRaised ? anim::TYPING_HAND_BAND * (1.5f + 1.5f * randUnit()) : 0.0f;
  servoAt(SERVO_HAND_RIGHT).setNormTarget(anim::TYPING_RIGHT_LOW + lift, g_gestureRaised ? 70.0f : 50.0f);
}

}  // namespace

void startTalking() {
  stopAnimServos();
  anim::parkNonePose();
  const uint32_t now = millis();
  g_poseFrozen = false;
  g_headUp = false;
  g_gestureRaised = false;
  g_headPauseUntilMs = now + randRangeMs(150, 350);
  g_neckNextMs = now + randRangeMs(400, 900);
  g_gestureNextMs = now + randRangeMs(1500, 3500);
  servoAt(SERVO_HEAD).setNormTarget(anim::TALKING_HEAD_MID, anim::TRANSITION_TORSO_SPEED_DEG_S);
}

void updateTalking(uint32_t now) {
  updateAllServos();

  if (hasPendingAnimation()) {
    if (!g_poseFrozen) {
      g_poseFrozen = true;
      anim::parkForTransition();
    }
    return;
  }

  if (g_poseFrozen) {
    // The pending switch was cancelled (talking requested again): resume from here.
    g_poseFrozen = false;
    g_headPauseUntilMs = now;
    g_neckNextMs = now;
    g_gestureNextMs = now + randRangeMs(1500, 3500);
  }

  if (!servoAt(SERVO_HEAD).isMoving() && now >= g_headPauseUntilMs) {
    commandHeadBeat();
    g_headPauseUntilMs = now + (g_headUp ? randRangeMs(60, 160) : randRangeMs(180, 650));
  }

  if (now >= g_neckNextMs) {
    commandNeckDrift();
    g_neckNextMs = now + randRangeMs(900, 2200);
  }

  if (now >= g_gestureNextMs && !servoAt(SERVO_HAND_RIGHT).isMoving()) {
    commandGesture();
    g_gestureNextMs = now + (g_gestureRaised ? randRangeMs(500, 1100) : randRangeMs(2500, 6000));
  }
}
