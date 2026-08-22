#include <Arduino.h>

#include "animation.h"
#include "animation/constants.h"
#include "animation/ring.h"
#include "animation/util.h"
#include "audio.h"
#include "eyes.h"
#include "servo_wrapper.h"
#include "servos.h"

using anim::stopAnimServos;

namespace {

constexpr float RING_STRIKE_SPEED_DEG_S = SERVO_MAX_SPEED_DEG_S;
constexpr float RING_BOUNCE_SPEED_DEG_S = 70.0f;
constexpr float RING_HEAD_STRIKE_SPEED_DEG_S = 150.0f;
constexpr float RING_START_SPEED_DEG_S = 120.0f;
constexpr float RING_RETURN_SPEED_DEG_S = SERVO_MAX_SPEED_DEG_S * 0.5f;

enum class RingPhase {
  ReachStart,
  Strike,
  Bounce,
  ReturnCenter,
  ReturnLeftHand,
  Done,
};

RingPhase g_ringPhase = RingPhase::ReachStart;

void commandStartPose() {
  servoAt(SERVO_BODY).setTarget(
    anim::RING_BODY_START,
    RING_START_SPEED_DEG_S
  );
  servoAt(SERVO_NECK).setTarget(
    anim::RING_NECK_START,
    RING_START_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    anim::RING_HEAD_START,
    RING_START_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_LEFT).setTarget(
    anim::RING_LEFT_START,
    RING_START_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::RING_RIGHT_START,
    RING_START_SPEED_DEG_S
  );
}

void commandStrike() {
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::RING_RIGHT_STRIKE,
    RING_STRIKE_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    anim::RING_HEAD_STRIKE,
    RING_HEAD_STRIKE_SPEED_DEG_S
  );
}

void commandBounce() {
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::RING_RIGHT_BOUNCE,
    RING_BOUNCE_SPEED_DEG_S
  );
}

void commandReturnCenter() {
  servoAt(SERVO_BODY).setTarget(
    servoMid(SERVO_SPECS[SERVO_BODY]),
    RING_RETURN_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_RIGHT).setTarget(
    SERVO_SPECS[SERVO_HAND_RIGHT].min,
    RING_RETURN_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    servoMid(SERVO_SPECS[SERVO_HEAD]),
    RING_RETURN_SPEED_DEG_S
  );
  servoAt(SERVO_NECK).setTarget(
    servoMid(SERVO_SPECS[SERVO_NECK]),
    RING_RETURN_SPEED_DEG_S
  );
}

void commandReturnLeftHand() {
  servoAt(SERVO_HAND_LEFT).setTarget(
    SERVO_SPECS[SERVO_HAND_LEFT].max,
    RING_RETURN_SPEED_DEG_S
  );
}

bool allStartServosStopped() {
  return !servoAt(SERVO_BODY).isMoving()
    && !servoAt(SERVO_NECK).isMoving()
    && !servoAt(SERVO_HEAD).isMoving()
    && !servoAt(SERVO_HAND_LEFT).isMoving()
    && !servoAt(SERVO_HAND_RIGHT).isMoving();
}

bool strikeServosStopped() {
  return !servoAt(SERVO_HAND_RIGHT).isMoving()
    && !servoAt(SERVO_HEAD).isMoving();
}

bool allServosStopped() {
  return allStartServosStopped();
}

}  // namespace

void startRing() {
  stopBellPlayback();
  stopWelcomePlayback();
  stopAnimServos();
  g_ringPhase = RingPhase::ReachStart;
  commandStartPose();
}

void updateRing(uint32_t now) {
  updateBellPlayback();
  updateAllServos();

  switch (g_ringPhase) {
    case RingPhase::ReachStart:
      if (allStartServosStopped()) {
        g_ringPhase = RingPhase::Strike;
        triggerEyeImpact(now);
        commandStrike();
      }
      break;

    case RingPhase::Strike:
      if (strikeServosStopped()) {
        startBellPlayback();
        g_ringPhase = RingPhase::Bounce;
        commandBounce();
      }
      break;

    case RingPhase::Bounce:
      if (!servoAt(SERVO_HAND_RIGHT).isMoving()) {
        g_ringPhase = RingPhase::ReturnCenter;
        commandReturnCenter();
      }
      break;

    case RingPhase::ReturnCenter:
      if (!servoAt(SERVO_BODY).isMoving()) {
        g_ringPhase = RingPhase::ReturnLeftHand;
        commandReturnLeftHand();
      }
      break;

    case RingPhase::ReturnLeftHand:
      if (allServosStopped()) {
        g_ringPhase = RingPhase::Done;
        finishAnimation(now);
      }
      break;

    case RingPhase::Done:
      break;
  }
}
