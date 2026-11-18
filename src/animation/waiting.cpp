#include <Arduino.h>

#include "animation/constants.h"
#include "animation/util.h"
#include "animation/waiting.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"
#include "serial_log.h"

using anim::beginEasedMove;
using anim::easedMoveDone;
using anim::easedMoveValue;
using anim::EasedMove;
using anim::randChance;
using anim::randRangeMs;
using anim::randUnit;

namespace {

// Torso turned to the robot's left, away from the bell (body + = left).
constexpr float WAIT_BODY = 0.8f;
constexpr float WAIT_SETTLE_SPEED_DEG_S = 30.0f;

// Left hand rests on the desk (+1, its scale is inverted) and lifts toward -
// a little for each tap.
constexpr int WAIT_TAP_SERVO = SERVO_HAND_LEFT;
constexpr float WAIT_HAND_REST = 1.0f;
constexpr float WAIT_TAP_DIR = -1.0f;
constexpr float WAIT_TAP_LIFT_MIN = 0.18f;
constexpr float WAIT_TAP_LIFT_SPAN = 0.10f;
constexpr float WAIT_TAP_UP_SPEED_DEG_S = 70.0f;
constexpr float WAIT_TAP_DOWN_SPEED_DEG_S = 100.0f;
constexpr uint32_t WAIT_TAP_GROUP_GAP_MIN_MS = 2500;
constexpr uint32_t WAIT_TAP_GROUP_GAP_MAX_MS = 8000;
constexpr uint32_t WAIT_TAP_IN_GROUP_GAP_MIN_MS = 90;
constexpr uint32_t WAIT_TAP_IN_GROUP_GAP_MAX_MS = 220;
// Every group is three taps; the groups start at random times.
constexpr uint8_t WAIT_TAPS_PER_GROUP = 3;

// Head corners. Neck: - = the robot's right, + = its left. Head: + = up.
constexpr float WAIT_HEAD_JITTER = 0.06f;
constexpr float WAIT_NECK_JITTER = 0.05f;
constexpr uint32_t WAIT_LOOK_HOLD_MIN_MS = 3000;
constexpr uint32_t WAIT_LOOK_HOLD_MAX_MS = 7000;
// Longer moves for the wide corners keep the head speed calm.
constexpr uint32_t WAIT_LOOK_MOVE_MIN_MS = 1400;
constexpr uint32_t WAIT_LOOK_MOVE_MAX_MS = 2300;
constexpr uint32_t WAIT_LOOK_STAGGER_MAX_MS = 200;

struct WaitCorner {
  const char* name;
  float headNorm;
  float neckNorm;
  float gazeX;  // eyes: -1 = robot's right, +1 = robot's left
  float gazeY;  // eyes: -1 = down, +1 = up
  uint8_t weight;
};

// Index 0 looks at the tapping hand: down and a little to the left.
constexpr WaitCorner WAIT_CORNERS[] = {
  {"hand (SW)", -0.90f, 0.30f, 0.6f, -1.0f, 34},
  {"up-left (NW)", 0.60f, 0.70f, 1.0f, 1.0f, 22},
  {"up-right (NE)", 0.60f, -0.70f, -1.0f, 1.0f, 22},
  {"down-right (SE)", -0.70f, -0.65f, -1.0f, -1.0f, 22},
};
constexpr uint8_t WAIT_CORNER_COUNT =
  sizeof(WAIT_CORNERS) / sizeof(WAIT_CORNERS[0]);
constexpr uint8_t WAIT_HAND_CORNER = 0;

enum class WaitPhase {
  Settle,
  Run,
};

enum class TapPhase {
  Idle,
  Lift,
  Drop,
  Gap,
};

WaitPhase g_phase = WaitPhase::Settle;
TapPhase g_tapPhase = TapPhase::Idle;
uint32_t g_nextTapMs = 0;
uint8_t g_tapsLeft = 0;

uint8_t g_corner = WAIT_HAND_CORNER;
uint32_t g_nextLookMs = 0;
EasedMove g_headMove = {};
EasedMove g_neckMove = {};
WaitingGaze g_gaze = {0.0f, 0.0f, 0};

float jitter(float amount) {
  return (randUnit() * 2.0f - 1.0f) * amount;
}

uint8_t pickNextCorner() {
  uint32_t total = 0;
  for (uint8_t i = 0; i < WAIT_CORNER_COUNT; i++) {
    if (i != g_corner) {
      total += WAIT_CORNERS[i].weight;
    }
  }

  uint32_t roll = randRangeMs(0, total - 1);
  for (uint8_t i = 0; i < WAIT_CORNER_COUNT; i++) {
    if (i == g_corner) {
      continue;
    }
    if (roll < WAIT_CORNERS[i].weight) {
      return i;
    }
    roll -= WAIT_CORNERS[i].weight;
  }
  return WAIT_HAND_CORNER;
}

void setGaze(uint8_t corner, uint32_t now) {
  g_gaze.x = WAIT_CORNERS[corner].gazeX;
  g_gaze.y = WAIT_CORNERS[corner].gazeY;
  g_gaze.sinceMs = now == 0 ? 1 : now;
}

void beginLook(uint8_t corner, uint32_t now) {
  const WaitCorner& c = WAIT_CORNERS[corner];
  const float headFrom =
    servoDegToNorm(SERVO_HEAD, servoAt(SERVO_HEAD).angle());
  const float neckFrom =
    servoDegToNorm(SERVO_NECK, servoAt(SERVO_NECK).angle());
  const float headTo = servoSaturateNorm(c.headNorm + jitter(WAIT_HEAD_JITTER));
  const float neckTo = servoSaturateNorm(c.neckNorm + jitter(WAIT_NECK_JITTER));

  // Eyes lead, then head and neck follow; one of them starts a bit later.
  const uint32_t stagger = randRangeMs(0, WAIT_LOOK_STAGGER_MAX_MS);
  const bool headFirst = randChance(50);
  beginEasedMove(
    g_headMove,
    headFrom,
    headTo,
    headFirst ? now : now + stagger,
    randRangeMs(WAIT_LOOK_MOVE_MIN_MS, WAIT_LOOK_MOVE_MAX_MS)
  );
  beginEasedMove(
    g_neckMove,
    neckFrom,
    neckTo,
    headFirst ? now + stagger : now,
    randRangeMs(WAIT_LOOK_MOVE_MIN_MS, WAIT_LOOK_MOVE_MAX_MS)
  );

  g_corner = corner;
  setGaze(corner, now);
  g_nextLookMs = now + randRangeMs(WAIT_LOOK_HOLD_MIN_MS, WAIT_LOOK_HOLD_MAX_MS);

  serialLogPrint("[anim] waiting look ");
  serialLogPrintln(c.name);
}

void tickLook(uint32_t now) {
  if (g_headMove.active) {
    servoAt(SERVO_HEAD).setNormPosition(easedMoveValue(g_headMove, now));
    if (easedMoveDone(g_headMove, now)) {
      g_headMove.active = false;
    }
  }
  if (g_neckMove.active) {
    servoAt(SERVO_NECK).setNormPosition(easedMoveValue(g_neckMove, now));
    if (easedMoveDone(g_neckMove, now)) {
      g_neckMove.active = false;
    }
  }

  const bool moving = g_headMove.active || g_neckMove.active;
  if (!moving && now >= g_nextLookMs) {
    beginLook(pickNextCorner(), now);
  }
}

void scheduleTapGroup(uint32_t now) {
  g_tapPhase = TapPhase::Idle;
  g_nextTapMs =
    now + randRangeMs(WAIT_TAP_GROUP_GAP_MIN_MS, WAIT_TAP_GROUP_GAP_MAX_MS);
}

void tickTaps(uint32_t now) {
  ServoWrapper& hand = servoAt(WAIT_TAP_SERVO);

  switch (g_tapPhase) {
    case TapPhase::Idle:
      if (now >= g_nextTapMs) {
        g_tapsLeft = WAIT_TAPS_PER_GROUP;
        g_tapPhase = TapPhase::Lift;
        hand.setNormTarget(
          WAIT_HAND_REST +
            WAIT_TAP_DIR * (WAIT_TAP_LIFT_MIN + WAIT_TAP_LIFT_SPAN * randUnit()),
          WAIT_TAP_UP_SPEED_DEG_S
        );
      }
      break;

    case TapPhase::Lift:
      if (!hand.isMoving()) {
        g_tapPhase = TapPhase::Drop;
        hand.setNormTarget(WAIT_HAND_REST, WAIT_TAP_DOWN_SPEED_DEG_S);
      }
      break;

    case TapPhase::Drop:
      if (!hand.isMoving()) {
        g_tapsLeft--;
        if (g_tapsLeft == 0) {
          scheduleTapGroup(now);
        } else {
          g_tapPhase = TapPhase::Gap;
          g_nextTapMs = now + randRangeMs(
            WAIT_TAP_IN_GROUP_GAP_MIN_MS,
            WAIT_TAP_IN_GROUP_GAP_MAX_MS
          );
        }
      }
      break;

    case TapPhase::Gap:
      if (now >= g_nextTapMs) {
        g_tapPhase = TapPhase::Lift;
        hand.setNormTarget(
          WAIT_HAND_REST +
            WAIT_TAP_DIR * (WAIT_TAP_LIFT_MIN + WAIT_TAP_LIFT_SPAN * randUnit()),
          WAIT_TAP_UP_SPEED_DEG_S
        );
      }
      break;
  }
}

bool settled() {
  return !servoAt(SERVO_BODY).isMoving()
    && !servoAt(SERVO_HAND_LEFT).isMoving()
    && !servoAt(SERVO_HAND_RIGHT).isMoving()
    && !servoAt(SERVO_HEAD).isMoving()
    && !servoAt(SERVO_NECK).isMoving();
}

}  // namespace

void startWaiting(uint32_t animationStartedMs) {
  serialLogPrintln("[anim] startWaiting");
  anim::stopAnimServos();

  // Turn to the left, park the hands, and look at the tapping hand.
  const WaitCorner& hand = WAIT_CORNERS[WAIT_HAND_CORNER];
  servoAt(SERVO_BODY).setNormTarget(WAIT_BODY, WAIT_SETTLE_SPEED_DEG_S);
  anim::parkHands(anim::TRANSITION_HAND_SPEED_DEG_S);
  servoAt(SERVO_HEAD).setNormTarget(hand.headNorm, WAIT_SETTLE_SPEED_DEG_S);
  servoAt(SERVO_NECK).setNormTarget(hand.neckNorm, WAIT_SETTLE_SPEED_DEG_S);

  g_phase = WaitPhase::Settle;
  g_tapPhase = TapPhase::Idle;
  g_tapsLeft = 0;
  g_headMove.active = false;
  g_neckMove.active = false;
  g_corner = WAIT_HAND_CORNER;
  setGaze(WAIT_HAND_CORNER, animationStartedMs);
}

void updateWaiting(uint32_t now) {
  servoAt(SERVO_BODY).update();
  servoAt(SERVO_HAND_LEFT).update();
  servoAt(SERVO_HAND_RIGHT).update();

  if (g_phase == WaitPhase::Settle) {
    servoAt(SERVO_HEAD).update();
    servoAt(SERVO_NECK).update();
    if (settled()) {
      g_phase = WaitPhase::Run;
      g_nextLookMs =
        now + randRangeMs(WAIT_LOOK_HOLD_MIN_MS, WAIT_LOOK_HOLD_MAX_MS);
      // The first tap comes soon, so the pose reads as "waiting" at once.
      g_nextTapMs = now + randRangeMs(600, 1500);
      serialLogPrintln("[anim] waiting settled");
    }
    return;
  }

  tickLook(now);
  tickTaps(now);
}

WaitingGaze waitingGaze() {
  return g_gaze;
}
