#include <Arduino.h>
#include <math.h>

#include "animation/constants.h"
#include "animation/thinking.h"
#include "animation/util.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"
#include "serial_log.h"

using anim::beginEasedMove;
using anim::easedMoveDone;
using anim::easedMoveValue;
using anim::EasedMove;
using anim::isTransitionParkComplete;
using anim::parkForTransition;
using anim::randChance;
using anim::randRangeMs;
using anim::randUnit;
using anim::stopAnimServos;

namespace {

constexpr float THINK_NECK_MID =
  servoMid(SERVO_SPECS[SERVO_NECK]);
constexpr float THINK_JITTER_DEG = 3.5f;
constexpr uint32_t THINK_MIN_POSE_CHANGE_MS = 2200;

enum class ThinkPhase {
  TransitionPark,
  PrimaryMove,
  Pause,
  MicroMove,
};

struct ThinkPose {
  float headDeg;
  float neckDeg;
};

ThinkPhase g_thinkPhase = ThinkPhase::TransitionPark;
EasedMove g_thinkHeadMove = {};
EasedMove g_thinkNeckMove = {};
uint32_t g_thinkPauseUntilMs = 0;
uint32_t g_animationStartedMs = 0;
uint8_t g_thinkPoseIndex = 0;
uint8_t g_thinkPoseChanges = 0;
bool g_thinkDidMicro = false;
uint8_t g_thinkMicroChain = 0;

constexpr ThinkPose THINK_POSES[] = {
  {125.0f, THINK_NECK_MID},
  {122.0f, THINK_NECK_MID - 8.0f},
  {120.0f, THINK_NECK_MID + 8.0f},
  {128.0f, THINK_NECK_MID - 5.0f},
  {127.0f, THINK_NECK_MID + 6.0f},
  {123.0f, THINK_NECK_MID - 10.0f},
  {121.0f, THINK_NECK_MID + 10.0f},
};
constexpr uint8_t THINK_POSE_COUNT =
  sizeof(THINK_POSES) / sizeof(THINK_POSES[0]);

float clampThinkHead(float deg) {
  return constrain(
    deg,
    SERVO_SPECS[SERVO_HEAD].min,
    SERVO_SPECS[SERVO_HEAD].max
  );
}

float clampThinkNeck(float deg) {
  return constrain(
    deg,
    SERVO_SPECS[SERVO_NECK].min,
    SERVO_SPECS[SERVO_NECK].max
  );
}

float jitterDeg() {
  return (randUnit() * 2.0f - 1.0f) * THINK_JITTER_DEG;
}

uint32_t jitterPauseMs(uint32_t baseMs) {
  const float factor = 0.75f + 0.5f * randUnit();
  return (uint32_t)(baseMs * factor);
}

ThinkPose perturbedPose(uint8_t index) {
  const ThinkPose& base = THINK_POSES[index % THINK_POSE_COUNT];
  ThinkPose pose;
  pose.headDeg = clampThinkHead(base.headDeg + jitterDeg());
  pose.neckDeg = clampThinkNeck(base.neckDeg + jitterDeg());
  return pose;
}

uint8_t pickNextThinkPoseIndex() {
  if (THINK_POSE_COUNT <= 1) {
    return g_thinkPoseIndex;
  }

  if (randChance(58)) {
    const float curHead = servoAt(SERVO_HEAD).angle();
    const float curNeck = servoAt(SERVO_NECK).angle();
    uint8_t bestIdx = g_thinkPoseIndex;
    float bestDist = 999.0f;
    uint8_t secondIdx = g_thinkPoseIndex;
    float secondDist = 999.0f;

    for (uint8_t i = 0; i < THINK_POSE_COUNT; i++) {
      if (i == g_thinkPoseIndex) {
        continue;
      }
      const ThinkPose& pose = THINK_POSES[i];
      const float dist = hypotf(
        pose.headDeg - curHead,
        pose.neckDeg - curNeck
      );
      if (dist < bestDist) {
        secondDist = bestDist;
        secondIdx = bestIdx;
        bestDist = dist;
        bestIdx = i;
      } else if (dist < secondDist) {
        secondDist = dist;
        secondIdx = i;
      }
    }

    if (secondIdx != g_thinkPoseIndex && randChance(35)) {
      return secondIdx;
    }
    return bestIdx;
  }

  uint8_t candidate = g_thinkPoseIndex;
  while (candidate == g_thinkPoseIndex) {
    candidate = (uint8_t)(esp_random() % THINK_POSE_COUNT);
  }
  return candidate;
}

void beginThinkAxisMove(
  EasedMove& move,
  float from,
  float to,
  uint32_t startMs,
  uint32_t durationMs
) {
  beginEasedMove(move, from, to, startMs, durationMs);
}

void tickThinkAxisMoves(uint32_t now) {
  if (g_thinkHeadMove.active) {
    servoAt(SERVO_HEAD).setPosition(
      easedMoveValue(g_thinkHeadMove, now)
    );
    if (easedMoveDone(g_thinkHeadMove, now)) {
      g_thinkHeadMove.active = false;
    }
  }
  if (g_thinkNeckMove.active) {
    servoAt(SERVO_NECK).setPosition(
      easedMoveValue(g_thinkNeckMove, now)
    );
    if (easedMoveDone(g_thinkNeckMove, now)) {
      g_thinkNeckMove.active = false;
    }
  }
}

uint32_t primaryDurationMs(float deltaDeg) {
  const uint32_t base = randRangeMs(480, 950);
  const float scale = 0.65f + 0.55f * (deltaDeg / 20.0f);
  return (uint32_t)(base * constrain(scale, 0.65f, 1.2f));
}

uint32_t microDurationMs() {
  return randRangeMs(300, 700);
}

void startThinkPrimaryMove(uint32_t now, const ThinkPose& pose) {
  const float headFrom = servoAt(SERVO_HEAD).angle();
  const float neckFrom = servoAt(SERVO_NECK).angle();
  const float headTo = pose.headDeg;
  const float neckTo = pose.neckDeg;

  const uint32_t headDur =
    primaryDurationMs(fabsf(headTo - headFrom));
  const uint32_t neckDur =
    primaryDurationMs(fabsf(neckTo - neckFrom));
  const uint32_t stagger = randRangeMs(100, 320);
  const bool headFirst = randChance(50);

  uint32_t headStart = now;
  uint32_t neckStart = now;
  if (headFirst) {
    neckStart += stagger;
  } else {
    headStart += stagger;
  }

  beginThinkAxisMove(
    g_thinkHeadMove,
    headFrom,
    headTo,
    headStart,
    headDur
  );
  beginThinkAxisMove(
    g_thinkNeckMove,
    neckFrom,
    neckTo,
    neckStart,
    neckDur
  );
  g_thinkPhase = ThinkPhase::PrimaryMove;
  g_thinkDidMicro = false;

  serialLogPrint("[anim] think primary head ");
  serialLogPrint(headFrom, 1);
  serialLogPrint("->");
  serialLogPrint(headTo, 1);
  serialLogPrint(" (");
  serialLogPrint(headDur);
  serialLogPrint("ms) neck ");
  serialLogPrint(neckFrom, 1);
  serialLogPrint("->");
  serialLogPrint(neckTo, 1);
  serialLogPrint(" (");
  serialLogPrint(neckDur);
  serialLogPrint("ms) headFirst=");
  serialLogPrint(headFirst ? "yes" : "no");
  serialLogPrint(" stagger=");
  serialLogPrintln(stagger);
}

bool thinkMovesActive() {
  return g_thinkHeadMove.active || g_thinkNeckMove.active;
}

void enterThinkPause(uint32_t now, bool afterMicro) {
  g_thinkPhase = ThinkPhase::Pause;
  if (afterMicro) {
    g_thinkPauseUntilMs =
      now + jitterPauseMs(randRangeMs(280, 850));
  } else {
    g_thinkPauseUntilMs =
      now + jitterPauseMs(randRangeMs(380, 1300));
  }
}

void beginThinkMicroMove(uint32_t now) {
  const float headFrom = servoAt(SERVO_HEAD).angle();
  const float neckFrom = servoAt(SERVO_NECK).angle();
  const float microDeg = 1.5f + 3.5f * randUnit();
  const uint32_t dur = microDurationMs();

  g_thinkHeadMove.active = false;
  g_thinkNeckMove.active = false;

  const uint8_t mode = (uint8_t)(esp_random() % 3u);
  if (mode == 0 || mode == 2) {
    const float sign = randChance(50) ? 1.0f : -1.0f;
    beginThinkAxisMove(
      g_thinkHeadMove,
      headFrom,
      clampThinkHead(headFrom + sign * microDeg),
      now,
      dur
    );
  }
  if (mode == 1 || mode == 2) {
    const float sign = randChance(50) ? 1.0f : -1.0f;
    beginThinkAxisMove(
      g_thinkNeckMove,
      neckFrom,
      clampThinkNeck(neckFrom + sign * microDeg),
      now + (mode == 2 ? randRangeMs(50, 150) : 0),
      dur
    );
  }
  g_thinkPhase = ThinkPhase::MicroMove;
  g_thinkDidMicro = true;
}

bool shouldChangeThinkPose(uint32_t now) {
  const uint32_t elapsed = now - g_animationStartedMs;
  if (elapsed < THINK_MIN_POSE_CHANGE_MS) {
    return false;
  }

  uint32_t chance = 44;
  if (elapsed >= 10000) {
    chance = 70;
  } else if (elapsed >= 5000) {
    chance = 58;
  }
  return randChance(chance);
}

void beginNextThinkPose(uint32_t now) {
  g_thinkPoseIndex = pickNextThinkPoseIndex();
  g_thinkPoseChanges++;
  startThinkPrimaryMove(now, perturbedPose(g_thinkPoseIndex));
}

}  // namespace

void startThinking(uint32_t animationStartedMs) {
  serialLogPrintln("[anim] startThinking");
  anim::logServoSnapshot("think-enter");
  stopAnimServos();
  anim::logServoSnapshot("think-post-stop");
  parkForTransition();
  g_animationStartedMs = animationStartedMs;

  g_thinkPhase = ThinkPhase::TransitionPark;
  g_thinkHeadMove.active = false;
  g_thinkNeckMove.active = false;
  g_thinkPoseIndex = pickNextThinkPoseIndex();
  g_thinkPoseChanges = 0;
  g_thinkDidMicro = false;
  g_thinkMicroChain = 0;
}

void updateThinking(uint32_t now) {
  servoAt(SERVO_HAND_LEFT).update();
  servoAt(SERVO_HAND_RIGHT).update();
  servoAt(SERVO_BODY).update();
  servoAt(SERVO_NECK).update();

  if (g_thinkPhase == ThinkPhase::TransitionPark) {
    if (isTransitionParkComplete()) {
      serialLogPrint("[anim] think transition park complete body=");
      serialLogPrint(servoAt(SERVO_BODY).angle(), 1);
      serialLogPrint(" head=");
      serialLogPrint(servoAt(SERVO_HEAD).angle(), 1);
      serialLogPrint(" neck=");
      serialLogPrintln(servoAt(SERVO_NECK).angle(), 1);
      startThinkPrimaryMove(now, perturbedPose(g_thinkPoseIndex));
    }
    return;
  }

  tickThinkAxisMoves(now);

  switch (g_thinkPhase) {
    case ThinkPhase::PrimaryMove:
      if (!thinkMovesActive()) {
        enterThinkPause(now, false);
      }
      break;

    case ThinkPhase::Pause:
      if (now < g_thinkPauseUntilMs) {
        break;
      }
      if (!g_thinkDidMicro && randChance(48)) {
        beginThinkMicroMove(now);
        break;
      }
      if (g_thinkDidMicro &&
          g_thinkMicroChain < 2 &&
          randChance(30)) {
        beginThinkMicroMove(now);
        g_thinkMicroChain++;
        break;
      }
      if (shouldChangeThinkPose(now)) {
        g_thinkMicroChain = 0;
        beginNextThinkPose(now);
      } else {
        g_thinkDidMicro = false;
        g_thinkMicroChain = 0;
        enterThinkPause(now, false);
      }
      break;

    case ThinkPhase::MicroMove:
      if (!thinkMovesActive()) {
        enterThinkPause(now, true);
      }
      break;
  }
}
