#include "animation/dead.h"

#include <Arduino.h>

#include "animation/constants.h"
#include "animation/error.h"
#include "animation/util.h"
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

void commandCollapsePose() {
  anim::parkTorso(anim::DEAD_COLLAPSE_SPEED_DEG_S);
  anim::parkHands(anim::DEAD_COLLAPSE_SPEED_DEG_S);
  servoAt(SERVO_HEAD).setTarget(
    anim::DEAD_HEAD_DOWN,
    anim::DEAD_COLLAPSE_SPEED_DEG_S
  );
}

void enterCollapseOrHold() {
  if (allDeadServosStopped()) {
    g_deadPhase = DeadPhase::Hold;
  } else {
    g_deadPhase = DeadPhase::Collapse;
  }
}

}  // namespace

void startDead() {
  g_deadPhase = DeadPhase::Warning;
  g_showingX = false;
  g_phaseStartedMs = 0;
  startError();
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
    case DeadPhase::Warning:
      if (updateErrorWarning(now)) {
        commandCollapsePose();
        g_phaseStartedMs = now;
        g_deadPhase = DeadPhase::Squeeze;
        eyes::requestForceRedraw();
      }
      break;

    case DeadPhase::Squeeze:
      updateAllServos();
      if ((now - g_phaseStartedMs) >= DEAD_SQUEEZE_MS) {
        g_phaseStartedMs = now;
        g_deadPhase = DeadPhase::Pause;
        eyes::requestForceRedraw();
      }
      break;

    case DeadPhase::Pause:
      updateAllServos();
      if ((now - g_phaseStartedMs) >= DEAD_PAUSE_MS) {
        g_showingX = true;
        eyes::requestForceRedraw();
        enterCollapseOrHold();
      }
      break;

    case DeadPhase::Collapse:
      updateAllServos();
      if (allDeadServosStopped()) {
        g_deadPhase = DeadPhase::Hold;
      }
      break;

    case DeadPhase::Hold:
      updateAllServos();
      break;
  }
}
