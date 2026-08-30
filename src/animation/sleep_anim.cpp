#include "animation/sleep_anim.h"

#include <Arduino.h>

#include "animation.h"
#include "animation/constants.h"
#include "hardware/servo_wrapper.h"
#include "pins.h"
#include "sleep.h"

namespace {

bool headAtSleepPose() {
  if (servoAt(SERVO_HEAD).isMoving()) {
    return false;
  }

  const float err =
    servoAt(SERVO_HEAD).angle() - anim::SLEEP_HEAD_DOWN;
  return err > -1.5f && err < 1.5f;
}

}  // namespace

void startSleepAnim() {
  requestSleep();
}

void updateSleepAnim(uint32_t nowMs) {
  updateAllServos();

  // Eyes may blank first; hold `sleep` until head settles chin-down
  // (same pose wakeup snaps to / rises from).
  if (isSleeping() && headAtSleepPose()) {
    finishAnimation(nowMs);
  }
}
