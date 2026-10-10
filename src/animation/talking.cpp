#include <Arduino.h>
#include <cmath>

#include "animation.h"
#include "animation/constants.h"
#include "animation/talking.h"
#include "animation/util.h"
#include "audio/audio_motion.h"
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
uint32_t g_bodyNextMs = 0;
uint32_t g_nodCooldownUntilMs = 0;
uint32_t g_lastPhraseId = 0;
bool g_wasStreamActive = false;

void commandAmbientHeadBeat() {
  g_headUp = !g_headUp;
  const float target = g_headUp
    ? anim::TALKING_HEAD_MID + anim::TALKING_HEAD_NOD_AMBIENT * (0.35f + 0.65f * randUnit())
    : anim::TALKING_HEAD_MID;
  const float speed = g_headUp ? 40.0f + 25.0f * randUnit() : 18.0f + 12.0f * randUnit();
  servoAt(SERVO_HEAD).setNormTarget(target, speed);
}

void commandNeckDrift() {
  const float target = anim::TALKING_NECK_SWAY * (2.0f * randUnit() - 1.0f);
  servoAt(SERVO_NECK).setNormTarget(target, 8.0f + 8.0f * randUnit());
}

void commandBodyDrift() {
  // Bias opposite the current neck so the face stays roughly aimed at the user.
  const float neckNorm = servoDegToNorm(SERVO_NECK, servoAt(SERVO_NECK).angle());
  float unit = 2.0f * randUnit() - 1.0f;
  if (neckNorm > 0.05f && unit > 0.0f) {
    unit = -unit;
  } else if (neckNorm < -0.05f && unit < 0.0f) {
    unit = -unit;
  }
  const float target = anim::TALKING_BODY_SWAY * unit;
  servoAt(SERVO_BODY).setNormTarget(target, 10.0f + 4.0f * randUnit());
}

void commandAmbientGesture() {
  g_gestureRaised = !g_gestureRaised;
  const float lift = g_gestureRaised ? anim::TALKING_HAND_LIFT_AMBIENT * (0.7f + 0.3f * randUnit()) : 0.0f;
  servoAt(SERVO_HAND_RIGHT).setNormTarget(
    anim::TYPING_RIGHT_LOW + lift,
    g_gestureRaised ? 60.0f : 45.0f
  );
}

void settleReactivePose() {
  g_headUp = false;
  if (!servoAt(SERVO_HEAD).isMoving() ||
      fabsf(servoDegToNorm(SERVO_HEAD, servoAt(SERVO_HEAD).angle()) - anim::TALKING_HEAD_MID) > 0.05f) {
    servoAt(SERVO_HEAD).setNormTarget(anim::TALKING_HEAD_MID, 22.0f);
  }
  if (g_gestureRaised) {
    g_gestureRaised = false;
    servoAt(SERVO_HAND_RIGHT).setNormTarget(anim::TYPING_RIGHT_LOW, 45.0f);
  }
  if (!servoAt(SERVO_BODY).isMoving() ||
      fabsf(servoDegToNorm(SERVO_BODY, servoAt(SERVO_BODY).angle())) > 0.03f) {
    servoAt(SERVO_BODY).setNormTarget(0.0f, 12.0f);
  }
}

void commandEmphasisNod(float emphasis) {
  g_headUp = true;
  const float amount = 0.45f + 0.55f * emphasis;
  const float target =
    anim::TALKING_HEAD_MID + anim::TALKING_HEAD_NOD * amount;
  const float speed = 50.0f + 40.0f * emphasis;
  servoAt(SERVO_HEAD).setNormTarget(target, speed);
}

void updateAmbientTalking(uint32_t now) {
  if (!servoAt(SERVO_HEAD).isMoving() && now >= g_headPauseUntilMs) {
    commandAmbientHeadBeat();
    g_headPauseUntilMs = now + (g_headUp ? randRangeMs(80, 180) : randRangeMs(280, 850));
  }

  if (now >= g_neckNextMs) {
    commandNeckDrift();
    g_neckNextMs = now + randRangeMs(1100, 2600);
  }

  if (now >= g_bodyNextMs && !servoAt(SERVO_BODY).isMoving()) {
    commandBodyDrift();
    g_bodyNextMs = now + randRangeMs(3000, 8000);
  }

  if (now >= g_gestureNextMs && !servoAt(SERVO_HAND_RIGHT).isMoving()) {
    commandAmbientGesture();
    g_gestureNextMs = now + (g_gestureRaised ? randRangeMs(600, 1300) : randRangeMs(3200, 7500));
  }
}

void updateReactiveTalking(uint32_t now, const AudioMotionSnapshot& snap) {
  if (now >= g_neckNextMs) {
    commandNeckDrift();
    g_neckNextMs = now + randRangeMs(1200, 2800);
  }

  if (!snap.speaking) {
    settleReactivePose();
    return;
  }

  // Hold near mid with a small loudness offset (not sample-rate bobbing).
  if (!g_headUp && !servoAt(SERVO_HEAD).isMoving()) {
    const float target =
      anim::TALKING_HEAD_MID + anim::TALKING_HEAD_LEVEL * snap.level;
    servoAt(SERVO_HEAD).setNormTarget(target, 28.0f);
  } else if (g_headUp && !servoAt(SERVO_HEAD).isMoving()) {
    g_headUp = false;
    const float target =
      anim::TALKING_HEAD_MID + anim::TALKING_HEAD_LEVEL * snap.level;
    servoAt(SERVO_HEAD).setNormTarget(target, 24.0f);
  }

  if (snap.emphasis >= anim::TALKING_EMPHASIS_NOD && now >= g_nodCooldownUntilMs) {
    commandEmphasisNod(snap.emphasis);
    g_nodCooldownUntilMs = now + anim::TALKING_NOD_COOLDOWN_MS;
  }

  if (snap.phraseId != g_lastPhraseId) {
    g_lastPhraseId = snap.phraseId;
    // Phrase start: hand XOR body — not both on the same beat.
    const bool raiseHand =
      !g_gestureRaised && !servoAt(SERVO_HAND_RIGHT).isMoving() && anim::randChance(55);
    if (raiseHand) {
      g_gestureRaised = true;
      servoAt(SERVO_HAND_RIGHT).setNormTarget(
        anim::TYPING_RIGHT_LOW + anim::TALKING_HAND_LIFT * (0.6f + 0.4f * randUnit()),
        70.0f
      );
      g_gestureNextMs = now + randRangeMs(700, 1600);
    } else if (!servoAt(SERVO_BODY).isMoving() && anim::randChance(40)) {
      commandBodyDrift();
      g_bodyNextMs = now + randRangeMs(4000, 9000);
    }
  } else if (g_gestureRaised && now >= g_gestureNextMs && !servoAt(SERVO_HAND_RIGHT).isMoving()) {
    g_gestureRaised = false;
    servoAt(SERVO_HAND_RIGHT).setNormTarget(anim::TYPING_RIGHT_LOW, 50.0f);
    g_gestureNextMs = now + randRangeMs(1800, 4500);
  } else if (
    !g_gestureRaised &&
    now >= g_gestureNextMs &&
    !servoAt(SERVO_HAND_RIGHT).isMoving() &&
    snap.level > 0.35f &&
    anim::randChance(18)
  ) {
    g_gestureRaised = true;
    servoAt(SERVO_HAND_RIGHT).setNormTarget(
      anim::TYPING_RIGHT_LOW + anim::TALKING_HAND_LIFT * (0.5f + 0.5f * randUnit()),
      65.0f
    );
    g_gestureNextMs = now + randRangeMs(600, 1400);
  }

  if (now >= g_bodyNextMs && !servoAt(SERVO_BODY).isMoving()) {
    commandBodyDrift();
    g_bodyNextMs = now + randRangeMs(4000, 9000);
  }
}

}  // namespace

void startTalking() {
  stopAnimServos();
  anim::parkNonePose();
  const uint32_t now = millis();
  g_poseFrozen = false;
  g_headUp = false;
  g_gestureRaised = false;
  g_headPauseUntilMs = now + randRangeMs(200, 450);
  g_neckNextMs = now + randRangeMs(500, 1100);
  g_gestureNextMs = now + randRangeMs(2000, 4500);
  g_bodyNextMs = now + randRangeMs(2500, 5000);
  g_nodCooldownUntilMs = now;
  g_lastPhraseId = 0;
  g_wasStreamActive = false;
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
    g_gestureNextMs = now + randRangeMs(2000, 4500);
    g_bodyNextMs = now + randRangeMs(2500, 5000);
    g_nodCooldownUntilMs = now;
  }

  const AudioMotionSnapshot snap = audioMotionSnapshot(now);

  if (snap.streamActive != g_wasStreamActive) {
    g_wasStreamActive = snap.streamActive;
    g_headUp = false;
    g_headPauseUntilMs = now;
    g_gestureNextMs = now + randRangeMs(800, 2000);
    g_bodyNextMs = now + randRangeMs(2500, 5000);
    if (snap.streamActive) {
      g_lastPhraseId = snap.phraseId;
      settleReactivePose();
    }
  }

  if (snap.streamActive) {
    updateReactiveTalking(now, snap);
  } else {
    updateAmbientTalking(now);
  }
}
