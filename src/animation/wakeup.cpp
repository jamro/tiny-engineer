#include "animation/wakeup.h"

#include <Arduino.h>
#include <cmath>

#include "animation/constants.h"
#include "animation/util.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/internal.h"
#include "hardware/servo_wrapper.h"
#include "pins.h"
#include "servos.h"

namespace {

constexpr uint32_t kOpenDurationMs = 2000;
constexpr uint32_t kTotalDurationMs = 5500;
constexpr uint32_t kSettleMs = 400;
constexpr float kNeckWaveAmpDeg = 4.0f;
constexpr float kWavePeriodMs = 2800.0f;
constexpr float kServoSpeedDegS = SERVO_BOOT_SPEED_DEG_S;
constexpr float kTwoPi = 6.28318530718f;

uint32_t g_startMs = 0;
bool g_moveServos = false;
bool g_finished = false;
bool g_settling = false;
uint32_t g_settleStartMs = 0;

float headMid() {
  return anim::SLEEP_HEAD_AWAKE;
}

float neckMid() {
  return servoMid(SERVO_SPECS[SERVO_NECK]);
}

void applyHeadNeck(float headDeg, float neckDeg) {
  servoAt(SERVO_HEAD).setTarget(headDeg, kServoSpeedDegS);
  servoAt(SERVO_NECK).setTarget(neckDeg, kServoSpeedDegS);
}

void snapSleepPose() {
  const float mid = neckMid();
  const float handRightDown = SERVO_SPECS[SERVO_HAND_RIGHT].min;
  const float handLeftDown = SERVO_SPECS[SERVO_HAND_LEFT].max;

  servoAt(SERVO_HEAD).setPosition(anim::SLEEP_HEAD_DOWN);
  servoAt(SERVO_NECK).setPosition(mid);
  servoAt(SERVO_HAND_RIGHT).setPosition(handRightDown);
  servoAt(SERVO_HAND_LEFT).setPosition(handLeftDown);
  servoAt(SERVO_BODY).setPosition(servoMid(SERVO_SPECS[SERVO_BODY]));
}

void markFinished(uint32_t now) {
  if (g_finished) {
    return;
  }

  g_finished = true;

  if (g_moveServos) {
    applyHeadNeck(headMid(), neckMid());
    servoAt(SERVO_HEAD).stop();
    servoAt(SERVO_NECK).stop();
  }

  blinkSetOpenAmount(1.0f);
  blinkBeginIdle(now);
  blinkSetNextBlinkMs(now + anim::randRangeMs(800, 2000));
  eyes::requestForceRedraw();
}

}  // namespace

void startWakeup(uint32_t nowMs, bool moveServos) {
  anim::stopAnimServos();

  g_startMs = nowMs;
  g_moveServos = moveServos;
  g_finished = false;
  g_settling = false;
  g_settleStartMs = 0;

  if (g_moveServos) {
    snapSleepPose();
  }
}

void updateWakeup(uint32_t nowMs) {
  if (g_finished) {
    return;
  }

  if (g_settling) {
    updateAllServos();

    if (nowMs - g_settleStartMs >= kSettleMs) {
      markFinished(nowMs);
    }

    return;
  }

  const uint32_t elapsed = nowMs - g_startMs;

  if (elapsed >= kTotalDurationMs) {
    if (g_moveServos) {
      applyHeadNeck(headMid(), neckMid());
      g_settling = true;
      g_settleStartMs = nowMs;
      updateAllServos();
    } else {
      markFinished(nowMs);
    }

    return;
  }

  if (g_moveServos) {
    const float riseT = anim::easeInOutCubic(
      static_cast<float>(elapsed) /
      static_cast<float>(kTotalDurationMs)
    );
    const float headDeg = anim::lerp(
      anim::SLEEP_HEAD_DOWN,
      headMid(),
      riseT
    );
    const float waveT =
      static_cast<float>(elapsed) / kWavePeriodMs;
    const float wave = sinf(waveT * kTwoPi);
    const float waveFade = 1.0f - riseT;
    const float neckDeg =
      neckMid() + wave * kNeckWaveAmpDeg * waveFade;

    applyHeadNeck(headDeg, neckDeg);
    updateAllServos();
  }
}

bool wakeupFinished() {
  return g_finished;
}
