#include "animation/dead.h"

#include <Arduino.h>
#include <math.h>

#include "animation/constants.h"
#include "animation/error.h"
#include "audio/audio.h"
#include "display/eyes/core/internal.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"

namespace {

enum class DeadPhase {
  Warning,
  Squeeze,
  Pause,
  Collapse,
  Hold,
};

DeadPhase g_deadPhase = DeadPhase::Warning;
bool g_showingX = false;
uint32_t g_phaseStartedMs = 0;

bool allDeadServosStopped() {
  return !servoAt(SERVO_BODY).isMoving()
    && !servoAt(SERVO_NECK).isMoving()
    && !servoAt(SERVO_HEAD).isMoving()
    && !servoAt(SERVO_HAND_LEFT).isMoving()
    && !servoAt(SERVO_HAND_RIGHT).isMoving();
}

float collapseSpeedDegS(int servo, float targetNorm) {
  const float from = servoAt(servo).angle();
  const float to = servoNormToDeg(servo, targetNorm);
  const float delta = fabsf(to - from);
  if (delta < 0.5f) {
    return 1.0f;
  }

  return delta * 1000.0f / static_cast<float>(DEAD_COLLAPSE_MS);
}

void commandCollapsePose() {
  servoAt(SERVO_BODY).setNormTarget(
    0.0f,
    collapseSpeedDegS(SERVO_BODY, 0.0f)
  );
  servoAt(SERVO_NECK).setNormTarget(
    0.0f,
    collapseSpeedDegS(SERVO_NECK, 0.0f)
  );
  servoAt(SERVO_HAND_RIGHT).setNormTarget(
    -1.0f,
    collapseSpeedDegS(SERVO_HAND_RIGHT, -1.0f)
  );
  servoAt(SERVO_HAND_LEFT).setNormTarget(
    1.0f,
    collapseSpeedDegS(SERVO_HAND_LEFT, 1.0f)
  );
  servoAt(SERVO_HEAD).setNormTarget(
    anim::DEAD_HEAD_DOWN,
    collapseSpeedDegS(SERVO_HEAD, anim::DEAD_HEAD_DOWN)
  );
}

void enterCollapseOrHold() {
  if (allDeadServosStopped()) {
    g_deadPhase = DeadPhase::Hold;
  } else {
    g_deadPhase = DeadPhase::Collapse;
  }
}

void beginCollapse(uint32_t now) {
  commandCollapsePose();
  g_phaseStartedMs = now;
  g_deadPhase = DeadPhase::Squeeze;
  eyes::requestForceRedraw();
}

void pumpDeadAudio() {
  updateDeadPlayback();
}

}  // namespace

void startDead() {
  g_deadPhase = DeadPhase::Warning;
  g_showingX = false;
  g_phaseStartedMs = 0;
  startDeadWarning();
}

bool deadShowingX() {
  return g_showingX;
}

bool deadIsSqueezing() {
  return g_deadPhase == DeadPhase::Squeeze;
}

bool deadIsPausedShut() {
  return g_deadPhase == DeadPhase::Pause;
}

uint32_t deadSqueezeStartedMs() {
  return g_phaseStartedMs;
}

void updateDead(uint32_t now) {
  switch (g_deadPhase) {
    case DeadPhase::Warning: {
      const bool warningDone = updateErrorWarning(now);
      const bool shutdownBeat = errorAudioStarted()
        && errorAudioElapsed(now) >= DEAD_AUDIO_SHUTDOWN_MS;
      if (warningDone || shutdownBeat) {
        beginCollapse(now);
      }
      break;
    }

    case DeadPhase::Squeeze:
      pumpDeadAudio();
      updateAllServos();
      if ((now - g_phaseStartedMs) >= DEAD_SQUEEZE_MS) {
        g_phaseStartedMs = now;
        g_deadPhase = DeadPhase::Pause;
        eyes::requestForceRedraw();
      }
      break;

    case DeadPhase::Pause:
      pumpDeadAudio();
      updateAllServos();
      if ((now - g_phaseStartedMs) >= DEAD_PAUSE_MS) {
        g_showingX = true;
        eyes::requestForceRedraw();
        enterCollapseOrHold();
      }
      break;

    case DeadPhase::Collapse:
      pumpDeadAudio();
      updateAllServos();
      if (allDeadServosStopped()) {
        g_deadPhase = DeadPhase::Hold;
      }
      break;

    case DeadPhase::Hold:
      pumpDeadAudio();
      updateAllServos();
      break;
  }
}
