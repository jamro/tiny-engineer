#include <Arduino.h>

#include "animation.h"
#include "animation/constants.h"
#include "animation/reading.h"
#include "animation/util.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"

using anim::parkTorso;
using anim::randRangeMs;
using anim::randUnit;
using anim::snapHeadToRangeHigh;
using anim::stopAnimServos;

namespace {

bool g_headHigh = true;
bool g_neckAngleHigh = true;
bool g_poseFrozen = false;
bool g_scrollPressing = false;
uint8_t g_scrollPressesLeft = 0;
uint32_t g_handPauseUntilMs = 0;
uint32_t g_headPauseUntilMs = 0;
uint32_t g_neckPauseUntilMs = 0;
uint32_t g_scrollIdleUntilMs = 0;

void commandReadingHead() {
  if (g_headHigh) {
    const float upSpeed = 12.0f + 10.0f * randUnit();
    servoAt(SERVO_HEAD).setTarget(anim::READING_HEAD_HIGH, upSpeed);
  } else {
    const float downSpeed = 2.5f + 2.5f * randUnit();
    servoAt(SERVO_HEAD).setTarget(anim::READING_HEAD_LOW, downSpeed);
  }
}

void commandReadingNeck() {
  const float speed = 8.0f + 8.0f * randUnit();
  if (g_neckAngleHigh) {
    servoAt(SERVO_NECK).setTarget(
      anim::READING_NECK_MID + anim::READING_NECK_SWAY_DEG,
      speed
    );
  } else {
    servoAt(SERVO_NECK).setTarget(
      anim::READING_NECK_MID - anim::READING_NECK_SWAY_DEG,
      speed
    );
  }
}

void commandScrollPress() {
  const float lift = 0.55f + 0.45f * randUnit();
  const float speedDegS =
    SERVO_MAX_SPEED_DEG_S * (0.85f + 0.15f * randUnit());
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::TYPING_RIGHT_LOW + lift * anim::TYPING_HAND_BAND_DEG,
    speedDegS
  );
}

void commandScrollRelease() {
  const float speedDegS =
    SERVO_MAX_SPEED_DEG_S * (0.55f + 0.20f * randUnit());
  servoAt(SERVO_HAND_RIGHT).setTarget(anim::TYPING_RIGHT_LOW, speedDegS);
}

void scheduleNextScrollBurst(uint32_t now) {
  g_scrollPressesLeft = 0;
  g_scrollPressing = false;
  g_handPauseUntilMs = 0;
  g_scrollIdleUntilMs = now + randRangeMs(3000, 8000);
}

void beginScrollBurst() {
  servoAt(SERVO_HEAD).stop();
  servoAt(SERVO_NECK).stop();
  g_headPauseUntilMs = 0;
  g_neckPauseUntilMs = 0;

  g_scrollPressesLeft = 1u + (uint8_t)(esp_random() % 3u);
  g_scrollPressing = true;
  g_handPauseUntilMs = 0;
  commandScrollPress();
}

void endScrollBurst(uint32_t now) {
  scheduleNextScrollBurst(now);
  commandReadingHead();
  commandReadingNeck();
}

void advanceReadingHeadStep() {
  g_headHigh = !g_headHigh;
  if (g_headHigh) {
    g_headPauseUntilMs = millis() + randRangeMs(400, 800);
  } else {
    g_headPauseUntilMs = millis() + randRangeMs(500, 1000);
  }
}

void beginNextReadingHead() {
  g_headPauseUntilMs = 0;
  commandReadingHead();
}

void advanceReadingNeckStep() {
  g_neckAngleHigh = !g_neckAngleHigh;
  if (g_neckAngleHigh) {
    g_neckPauseUntilMs = millis() + randRangeMs(350, 700);
  } else {
    g_neckPauseUntilMs = millis() + randRangeMs(450, 900);
  }
}

void beginNextReadingNeck() {
  g_neckPauseUntilMs = 0;
  commandReadingNeck();
}

}  // namespace

void startReading() {
  stopAnimServos();
  anim::parkNonePose();
  g_poseFrozen = false;
  g_neckAngleHigh = anim::randChance(50);
  g_headHigh = true;
  g_headPauseUntilMs = 0;
  g_neckPauseUntilMs = 0;
  snapHeadToRangeHigh(anim::READING_HEAD_HIGH);
  commandReadingNeck();
  scheduleNextScrollBurst(millis());
}

void updateReading(uint32_t now) {
  ServoWrapper& head = servoAt(SERVO_HEAD);
  ServoWrapper& neck = servoAt(SERVO_NECK);
  ServoWrapper& right = servoAt(SERVO_HAND_RIGHT);

  updateAllServos();

  if (hasPendingAnimation() && !g_poseFrozen) {
    g_poseFrozen = true;
    g_headPauseUntilMs = 0;
    g_neckPauseUntilMs = 0;
    parkTorso(anim::TRANSITION_TORSO_SPEED_DEG_S);
  }

  const bool scrolling = g_scrollPressesLeft > 0;

  if (!scrolling && !g_poseFrozen) {
    if (g_headPauseUntilMs != 0) {
      if (now >= g_headPauseUntilMs) {
        beginNextReadingHead();
      }
    } else if (!head.isMoving()) {
      advanceReadingHeadStep();
    }

    if (g_neckPauseUntilMs != 0) {
      if (now >= g_neckPauseUntilMs) {
        beginNextReadingNeck();
      }
    } else if (!neck.isMoving()) {
      advanceReadingNeckStep();
    }
  }

  if (scrolling) {
    if (g_handPauseUntilMs != 0) {
      if (now >= g_handPauseUntilMs) {
        g_handPauseUntilMs = 0;
        g_scrollPressing = true;
        commandScrollPress();
      }
    } else if (!right.isMoving()) {
      if (g_scrollPressing) {
        g_scrollPressing = false;
        commandScrollRelease();
      } else {
        g_scrollPressesLeft--;
        if (g_scrollPressesLeft > 0) {
          g_handPauseUntilMs = millis() + randRangeMs(55, 160);
        } else {
          endScrollBurst(now);
        }
      }
    }
  } else if (now >= g_scrollIdleUntilMs) {
    beginScrollBurst();
  }
}
