#include <Arduino.h>
#include <string.h>

#include "animation.h"
#include "servos.h"
#include "servo_wrapper.h"

namespace {

AnimationId g_animation = AnimationId::None;
uint32_t g_animationStartedMs = 0;
bool g_hasPendingAnimation = false;
AnimationId g_pendingAnimation = AnimationId::None;

constexpr uint32_t MIN_ANIMATION_HOLD_MS = 1000;

// Right/left scales inverted: same numeric direction = opposite physical.
// Hands alternate with randomness (pauses, hand choice, speed, stroke depth).
// Head: lowest 10° of scale; slow down, fast up (follow text).
// Body ±5° around mid; neck opposite so head stays put.
bool g_typingMoveRight = true;
bool g_headHigh = true;
bool g_bodySwayPositive = true;
bool g_neckAngleHigh = true;  // reading: true = left (angle up)
bool g_scrollPressing = false;
uint8_t g_scrollPressesLeft = 0;
uint32_t g_handPauseUntilMs = 0;
uint32_t g_headPauseUntilMs = 0;
uint32_t g_swayPauseUntilMs = 0;
uint32_t g_neckPauseUntilMs = 0;
uint32_t g_scrollIdleUntilMs = 0;

constexpr float TYPING_HAND_BAND_DEG = 15.0f;
constexpr float TYPING_HEAD_BAND_DEG = 10.0f;
constexpr float TYPING_SWAY_DEG = 5.0f;
constexpr float READING_HEAD_BAND_DEG = 10.0f;
constexpr float READING_NECK_SWAY_DEG = 10.0f;
constexpr float THINKING_NECK_SWAY_DEG = 25.0f;
constexpr float THINKING_HEAD_DEG = 130.0f;

constexpr float TYPING_RIGHT_LOW =
  SERVO_SPECS[SERVO_HAND_RIGHT].min;
constexpr float TYPING_RIGHT_HIGH =
  SERVO_SPECS[SERVO_HAND_RIGHT].min + TYPING_HAND_BAND_DEG;
constexpr float TYPING_LEFT_LOW =
  SERVO_SPECS[SERVO_HAND_LEFT].max - TYPING_HAND_BAND_DEG;
constexpr float TYPING_LEFT_HIGH =
  SERVO_SPECS[SERVO_HAND_LEFT].max;

constexpr float TYPING_HEAD_LOW =
  SERVO_SPECS[SERVO_HEAD].min;
constexpr float TYPING_HEAD_HIGH =
  SERVO_SPECS[SERVO_HEAD].min + TYPING_HEAD_BAND_DEG;

constexpr float TYPING_BODY_MID =
  servoMid(SERVO_SPECS[SERVO_BODY]);
constexpr float TYPING_NECK_MID =
  servoMid(SERVO_SPECS[SERVO_NECK]);

constexpr float READING_HEAD_LOW =
  SERVO_SPECS[SERVO_HEAD].min;
constexpr float READING_HEAD_HIGH =
  SERVO_SPECS[SERVO_HEAD].min + READING_HEAD_BAND_DEG;
constexpr float READING_NECK_MID =
  servoMid(SERVO_SPECS[SERVO_NECK]);

constexpr float THINKING_NECK_MID =
  servoMid(SERVO_SPECS[SERVO_NECK]);
constexpr float THINKING_NECK_LOW =
  THINKING_NECK_MID - THINKING_NECK_SWAY_DEG;
constexpr float THINKING_NECK_HIGH =
  THINKING_NECK_MID + THINKING_NECK_SWAY_DEG;

float randUnit() {
  return (float)(esp_random() & 0xFFFFu) / 65535.0f;
}

bool randChance(uint32_t percent) {
  return (esp_random() % 100u) < percent;
}

uint32_t randRangeMs(uint32_t lo, uint32_t hi) {
  if (hi <= lo) {
    return lo;
  }
  return lo + (esp_random() % (hi - lo + 1u));
}

void stopAnimServos() {
  servoAt(SERVO_HAND_LEFT).stop();
  servoAt(SERVO_HAND_RIGHT).stop();
  servoAt(SERVO_HEAD).stop();
  servoAt(SERVO_NECK).stop();
  servoAt(SERVO_BODY).stop();
}

void parkNonePose() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (i == SERVO_HAND_LEFT || i == SERVO_HAND_RIGHT) {
      continue;
    }
    servoAt(i).setTarget(servoMid(SERVO_SPECS[i]));
  }

  // Inverted hand scales: down = right min, left max.
  servoAt(SERVO_HAND_RIGHT).setTarget(
    SERVO_SPECS[SERVO_HAND_RIGHT].min
  );
  servoAt(SERVO_HAND_LEFT).setTarget(
    SERVO_SPECS[SERVO_HAND_LEFT].max
  );
}

void commandBodyNeckSway() {
  const float offset =
    g_bodySwayPositive ? TYPING_SWAY_DEG : -TYPING_SWAY_DEG;
  // Same speed so neck cancels body and head stays put.
  const float speedDegS =
    12.0f + 18.0f * randUnit();

  servoAt(SERVO_BODY).setTarget(
    TYPING_BODY_MID + offset,
    speedDegS
  );
  servoAt(SERVO_NECK).setTarget(
    TYPING_NECK_MID - offset,
    speedDegS
  );
}

void commandHandStroke() {
  // Prefer opposite hand; frequent same-hand bursts (steroid typing).
  if (!randChance(45)) {
    g_typingMoveRight = !g_typingMoveRight;
  }

  // Short, snappy strokes near band ends.
  const float depth = 0.35f + 0.45f * randUnit();
  const bool pressHigh = randChance(50);
  const float speedDegS =
    SERVO_MAX_SPEED_DEG_S * (0.75f + 0.25f * randUnit());

  if (g_typingMoveRight) {
    const float target = pressHigh
      ? TYPING_RIGHT_LOW + depth * TYPING_HAND_BAND_DEG
      : TYPING_RIGHT_HIGH - depth * TYPING_HAND_BAND_DEG;
    servoAt(SERVO_HAND_RIGHT).setTarget(target, speedDegS);
  } else {
    const float target = pressHigh
      ? TYPING_LEFT_LOW + depth * TYPING_HAND_BAND_DEG
      : TYPING_LEFT_HIGH - depth * TYPING_HAND_BAND_DEG;
    servoAt(SERVO_HAND_LEFT).setTarget(target, speedDegS);
  }
}

void commandHead() {
  if (g_headHigh) {
    // Up still faster than down, but not frantic.
    const float upSpeed =
      35.0f + 25.0f * randUnit();
    servoAt(SERVO_HEAD).setTarget(
      TYPING_HEAD_HIGH,
      upSpeed
    );
  } else {
    const float downSpeed =
      6.0f + 6.0f * randUnit();
    servoAt(SERVO_HEAD).setTarget(
      TYPING_HEAD_LOW,
      downSpeed
    );
  }
}

void commandReadingHead() {
  if (g_headHigh) {
    const float upSpeed = 12.0f + 10.0f * randUnit();
    servoAt(SERVO_HEAD).setTarget(READING_HEAD_HIGH, upSpeed);
  } else {
    const float downSpeed = 2.5f + 2.5f * randUnit();
    servoAt(SERVO_HEAD).setTarget(READING_HEAD_LOW, downSpeed);
  }
}

void commandReadingNeck() {
  // Left = angle increase (faster); right = angle decrease (slower).
  if (g_neckAngleHigh) {
    const float leftSpeed = 14.0f + 12.0f * randUnit();
    servoAt(SERVO_NECK).setTarget(
      READING_NECK_MID + READING_NECK_SWAY_DEG,
      leftSpeed
    );
  } else {
    const float rightSpeed = 5.0f + 5.0f * randUnit();
    servoAt(SERVO_NECK).setTarget(
      READING_NECK_MID - READING_NECK_SWAY_DEG,
      rightSpeed
    );
  }
}

void commandScrollPress() {
  // Same 15° right-hand band as typing; press toward high end.
  const float depth = 0.55f + 0.45f * randUnit();
  const float speedDegS =
    SERVO_MAX_SPEED_DEG_S * (0.75f + 0.25f * randUnit());
  servoAt(SERVO_HAND_RIGHT).setTarget(
    TYPING_RIGHT_LOW + depth * TYPING_HAND_BAND_DEG,
    speedDegS
  );
}

void commandScrollRelease() {
  const float speedDegS =
    SERVO_MAX_SPEED_DEG_S * (0.75f + 0.25f * randUnit());
  servoAt(SERVO_HAND_RIGHT).setTarget(TYPING_RIGHT_LOW, speedDegS);
}

void scheduleNextScrollBurst(uint32_t now) {
  g_scrollPressesLeft = 0;
  g_scrollPressing = false;
  g_handPauseUntilMs = 0;
  g_scrollIdleUntilMs = now + randRangeMs(3000, 8000);
}

void beginScrollBurst() {
  // Hold head/neck still while right hand scrolls.
  servoAt(SERVO_HEAD).stop();
  servoAt(SERVO_NECK).stop();
  g_headPauseUntilMs = 0;
  g_neckPauseUntilMs = 0;

  g_scrollPressesLeft = 1u + (uint8_t)(esp_random() % 3u);  // 1..3
  g_scrollPressing = true;
  g_handPauseUntilMs = 0;
  commandScrollPress();
}

void endScrollBurst(uint32_t now) {
  scheduleNextScrollBurst(now);
  // Resume bob/sweep from same direction flags.
  commandReadingHead();
  commandReadingNeck();
}

void snapHeadToRangeHigh(float highDeg) {
  // Intro: jump to band top fast, then normal bob continues.
  g_headHigh = true;
  g_headPauseUntilMs = 0;
  servoAt(SERVO_HEAD).setTarget(
    highDeg,
    SERVO_MAX_SPEED_DEG_S * 0.85f
  );
}

void startTyping() {
  stopAnimServos();
  g_typingMoveRight = randChance(50);
  g_bodySwayPositive = randChance(50);
  g_scrollPressesLeft = 0;
  g_scrollPressing = false;
  g_handPauseUntilMs = 0;
  g_swayPauseUntilMs = 0;
  g_neckPauseUntilMs = 0;
  g_scrollIdleUntilMs = 0;
  commandBodyNeckSway();
  commandHandStroke();
  snapHeadToRangeHigh(TYPING_HEAD_HIGH);
}

void startReading() {
  stopAnimServos();
  parkNonePose();
  g_neckAngleHigh = randChance(50);
  g_swayPauseUntilMs = 0;
  g_neckPauseUntilMs = 0;
  snapHeadToRangeHigh(READING_HEAD_HIGH);
  commandReadingNeck();
  scheduleNextScrollBurst(millis());
}

void startNone() {
  stopAnimServos();
  parkNonePose();
  g_scrollPressesLeft = 0;
  g_scrollPressing = false;
  g_handPauseUntilMs = 0;
  g_headPauseUntilMs = 0;
  g_swayPauseUntilMs = 0;
  g_neckPauseUntilMs = 0;
  g_scrollIdleUntilMs = 0;
}

void commandThinkingNeck() {
  const float speedDegS = 4.0f + 4.0f * randUnit();
  const float target = g_neckAngleHigh
    ? THINKING_NECK_HIGH
    : THINKING_NECK_LOW;
  servoAt(SERVO_NECK).setTarget(target, speedDegS);
}

void startThinking() {
  stopAnimServos();
  parkNonePose();
  g_scrollPressesLeft = 0;
  g_scrollPressing = false;
  g_handPauseUntilMs = 0;
  g_headPauseUntilMs = 0;
  g_swayPauseUntilMs = 0;
  g_scrollIdleUntilMs = 0;
  g_neckAngleHigh = true;
  g_neckPauseUntilMs = 0;
  servoAt(SERVO_HEAD).setTarget(THINKING_HEAD_DEG);
  commandThinkingNeck();
}

void advanceTypingStep() {
  // Near-zero gaps; rare tiny breath.
  const uint32_t pauseMs = randChance(8)
    ? randRangeMs(40, 90)
    : randRangeMs(0, 25);
  g_handPauseUntilMs = millis() + pauseMs;
}

void beginNextHandStroke() {
  g_handPauseUntilMs = 0;
  commandHandStroke();
}

void advanceHeadStep() {
  g_headHigh = !g_headHigh;

  if (g_headHigh) {
    g_headPauseUntilMs = millis() + randRangeMs(80, 220);
  } else {
    g_headPauseUntilMs = millis() + randRangeMs(120, 350);
  }
}

void beginNextHeadMove() {
  g_headPauseUntilMs = 0;
  commandHead();
}

void advanceSwayStep() {
  g_bodySwayPositive = !g_bodySwayPositive;
  g_swayPauseUntilMs = millis() + randRangeMs(40, 160);
}

void beginNextSway() {
  g_swayPauseUntilMs = 0;
  commandBodyNeckSway();
}

void advanceReadingHeadStep() {
  g_headHigh = !g_headHigh;
  if (g_headHigh) {
    g_headPauseUntilMs = millis() + randRangeMs(200, 500);
  } else {
    g_headPauseUntilMs = millis() + randRangeMs(300, 700);
  }
}

void beginNextReadingHead() {
  g_headPauseUntilMs = 0;
  commandReadingHead();
}

void advanceReadingNeckStep() {
  g_neckAngleHigh = !g_neckAngleHigh;
  // Linger longer after slow right sweep (end of line).
  if (g_neckAngleHigh) {
    g_neckPauseUntilMs = millis() + randRangeMs(150, 400);
  } else {
    g_neckPauseUntilMs = millis() + randRangeMs(250, 600);
  }
}

void beginNextReadingNeck() {
  g_neckPauseUntilMs = 0;
  commandReadingNeck();
}

void advanceThinkingNeckStep() {
  g_neckAngleHigh = !g_neckAngleHigh;
  g_neckPauseUntilMs = millis() + randRangeMs(200, 600);
}

void beginNextThinkingNeck() {
  g_neckPauseUntilMs = 0;
  commandThinkingNeck();
}

void applyAnimation(AnimationId id) {
  g_animation = id;
  g_animationStartedMs = millis();
  g_hasPendingAnimation = false;

  switch (id) {
    case AnimationId::Typing:
      startTyping();
      break;
    case AnimationId::Reading:
      startReading();
      break;
    case AnimationId::Thinking:
      startThinking();
      break;
    case AnimationId::None:
    default:
      startNone();
      break;
  }
}

bool animationHoldElapsed() {
  return (millis() - g_animationStartedMs) >= MIN_ANIMATION_HOLD_MS;
}

}  // namespace

void setAnimation(AnimationId id) {
  if (id == g_animation) {
    g_hasPendingAnimation = false;
    return;
  }

  if (animationHoldElapsed()) {
    applyAnimation(id);
    return;
  }

  g_pendingAnimation = id;
  g_hasPendingAnimation = true;
}

AnimationId getAnimation() {
  return g_animation;
}

const char* animationName(AnimationId id) {
  switch (id) {
    case AnimationId::Typing:
      return "typing";
    case AnimationId::Reading:
      return "reading";
    case AnimationId::Thinking:
      return "thinking";
    case AnimationId::None:
    default:
      return "none";
  }
}

bool parseAnimationName(const char* name, AnimationId& out) {
  if (name == nullptr) {
    return false;
  }

  if (strcmp(name, "none") == 0) {
    out = AnimationId::None;
    return true;
  }

  if (strcmp(name, "typing") == 0) {
    out = AnimationId::Typing;
    return true;
  }

  if (strcmp(name, "reading") == 0) {
    out = AnimationId::Reading;
    return true;
  }

  if (strcmp(name, "thinking") == 0) {
    out = AnimationId::Thinking;
    return true;
  }

  return false;
}

void updateTyping(uint32_t now) {
  ServoWrapper& left = servoAt(SERVO_HAND_LEFT);
  ServoWrapper& right = servoAt(SERVO_HAND_RIGHT);
  ServoWrapper& head = servoAt(SERVO_HEAD);
  ServoWrapper& neck = servoAt(SERVO_NECK);
  ServoWrapper& body = servoAt(SERVO_BODY);

  left.update();
  right.update();
  head.update();
  neck.update();
  body.update();

  if (g_handPauseUntilMs != 0) {
    if (now >= g_handPauseUntilMs) {
      beginNextHandStroke();
    }
  } else {
    const bool activeDone = g_typingMoveRight
      ? !right.isMoving()
      : !left.isMoving();

    if (activeDone) {
      advanceTypingStep();
    }
  }

  if (g_headPauseUntilMs != 0) {
    if (now >= g_headPauseUntilMs) {
      beginNextHeadMove();
    }
  } else if (!head.isMoving()) {
    advanceHeadStep();
  }

  if (g_swayPauseUntilMs != 0) {
    if (now >= g_swayPauseUntilMs) {
      beginNextSway();
    }
  } else if (!body.isMoving() && !neck.isMoving()) {
    advanceSwayStep();
  }
}

void updateReading(uint32_t now) {
  ServoWrapper& head = servoAt(SERVO_HEAD);
  ServoWrapper& neck = servoAt(SERVO_NECK);
  ServoWrapper& right = servoAt(SERVO_HAND_RIGHT);

  // Body/left stay in none pose; tick all so park + scroll finish.
  updateAllServos();

  const bool scrolling = g_scrollPressesLeft > 0;

  if (!scrolling) {
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

  // Occasional right-hand down-arrow bursts (1–3 presses).
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
          g_handPauseUntilMs = millis() + randRangeMs(40, 120);
        } else {
          endScrollBurst(now);
        }
      }
    }
  } else if (now >= g_scrollIdleUntilMs) {
    beginScrollBurst();
  }
}

void updateThinking(uint32_t now) {
  ServoWrapper& neck = servoAt(SERVO_NECK);

  // Hands/body/head stay in thinking park; tick all so moves finish.
  updateAllServos();

  if (g_neckPauseUntilMs != 0) {
    if (now >= g_neckPauseUntilMs) {
      beginNextThinkingNeck();
    }
  } else if (!neck.isMoving()) {
    advanceThinkingNeckStep();
  }
}

void updateAnimation() {
  if (g_hasPendingAnimation && animationHoldElapsed()) {
    applyAnimation(g_pendingAnimation);
  }

  if (g_animation == AnimationId::None) {
    updateAllServos();
    return;
  }

  const uint32_t now = millis();

  if (g_animation == AnimationId::Typing) {
    updateTyping(now);
    return;
  }

  if (g_animation == AnimationId::Reading) {
    updateReading(now);
    return;
  }

  if (g_animation == AnimationId::Thinking) {
    updateThinking(now);
  }
}
