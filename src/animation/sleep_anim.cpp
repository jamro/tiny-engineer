#include "animation/sleep_anim.h"

#include <Arduino.h>

#include "animation.h"
#include "animation/constants.h"
#include "animation/util.h"
#include "hardware/servo_wrapper.h"
#include "sleep.h"

namespace {

constexpr uint32_t kDroopEndMs = 700;
constexpr uint32_t kResistEndMs = 1100;
constexpr float kDroopOpen = 0.15f;
constexpr float kResistOpen = 0.5f;
// Fraction of the way from the start pose to chin-down.
constexpr float kHeadHint = 0.08f;
constexpr float kHeadNod = 0.40f;

uint32_t g_startedMs = 0;
float g_fromHead = 0.0f;

float clamp01(float t) {
  if (t <= 0.0f) {
    return 0.0f;
  }
  if (t >= 1.0f) {
    return 1.0f;
  }
  return t;
}

float easeOutCubic(float t) {
  t = clamp01(t);
  const float f = 1.0f - t;
  return 1.0f - f * f * f;
}

float segmentT(uint32_t elapsed, uint32_t start, uint32_t end) {
  if (elapsed <= start || end <= start) {
    return 0.0f;
  }
  if (elapsed >= end) {
    return 1.0f;
  }
  return (float)(elapsed - start) / (float)(end - start);
}

float headAlong(float fromHead, float u) {
  return anim::lerp(fromHead, anim::SLEEP_HEAD_DOWN, clamp01(u));
}

bool headAtSleepPose() {
  if (servoAt(SERVO_HEAD).isMoving()) {
    return false;
  }

  const float err =
    servoAt(SERVO_HEAD).angle() -
    servoNormToDeg(SERVO_HEAD, anim::SLEEP_HEAD_DOWN);
  return err > -1.5f && err < 1.5f;
}

}  // namespace

namespace anim {

SleepNodOffPose sleepNodOffPose(
  uint32_t elapsedMs,
  float fromOpen,
  float fromHead
) {
  SleepNodOffPose pose;

  if (elapsedMs >= SLEEP_NOD_OFF_MS) {
    pose.open = 0.0f;
    pose.head = SLEEP_HEAD_DOWN;
    return pose;
  }

  if (elapsedMs < kDroopEndMs) {
    const float t = easeInOutCubic(segmentT(elapsedMs, 0, kDroopEndMs));
    pose.open = clamp01(lerp(fromOpen, kDroopOpen, t));
    pose.head = headAlong(fromHead, lerp(0.0f, kHeadHint, t));
    return pose;
  }

  if (elapsedMs < kResistEndMs) {
    const float t = easeInOutCubic(
      segmentT(elapsedMs, kDroopEndMs, kResistEndMs)
    );
    pose.open = lerp(kDroopOpen, kResistOpen, t);
    pose.head = headAlong(fromHead, lerp(kHeadHint, kHeadNod, t));
    return pose;
  }

  const float t = easeOutCubic(
    segmentT(elapsedMs, kResistEndMs, SLEEP_NOD_OFF_MS)
  );
  pose.open = lerp(kResistOpen, 0.0f, t);
  pose.head = headAlong(fromHead, lerp(kHeadNod, 1.0f, t));
  return pose;
}

}  // namespace anim

void startSleepAnim(uint32_t nowMs) {
  g_startedMs = nowMs;
  g_fromHead = servoDegToNorm(SERVO_HEAD, servoAt(SERVO_HEAD).angle());
  requestSleep(nowMs);
}

void updateSleepAnim(uint32_t nowMs) {
  const uint32_t elapsed =
    nowMs >= g_startedMs ? nowMs - g_startedMs : 0;
  const anim::SleepNodOffPose pose = anim::sleepNodOffPose(
    elapsed,
    0.0f,
    g_fromHead
  );

  // Fast enough to track the curve; the ease is the motion.
  servoAt(SERVO_HEAD).setNormTarget(pose.head, SERVO_MAX_SPEED_DEG_S);
  updateAllServos();

  // Eyes may blank first; hold `sleep` until head settles chin-down
  // (same pose wakeup snaps to / rises from).
  if (isSleeping() && headAtSleepPose()) {
    finishAnimation(nowMs);
  }
}
