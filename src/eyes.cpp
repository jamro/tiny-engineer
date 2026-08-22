#include <Arduino.h>

#include "animation/util.h"
#include "animation/welcome.h"
#include "eyes.h"
#include "oled.h"

namespace {

constexpr int EYE_CORNER_RADIUS = 3;
constexpr float BLINK_CLOSED_AMOUNT = 0.12f;
constexpr uint32_t REDRAW_INTERVAL_MS = 33;
constexpr uint32_t DOUBLE_BLINK_CHANCE_PERCENT = 22;

constexpr Eye DEFAULT_LEFT = {20, 9, 24, 14};
constexpr Eye DEFAULT_RIGHT = {84, 9, 24, 14};
constexpr int16_t EYE_CENTER_Y =
  DEFAULT_LEFT.y + DEFAULT_LEFT.height / 2;

enum class BlinkPhase {
  Idle,
  Closing,
  Closed,
  Opening
};

enum class SleepEyeAnim {
  None,
  Closing,
  Opening
};

enum class ReadingScanPhase {
  Forward,
  Return
};

Eye g_leftEye = DEFAULT_LEFT;
Eye g_rightEye = DEFAULT_RIGHT;

bool g_eyesActive = false;
EyeMode g_eyeMode = EyeMode::Idle;
uint32_t g_modeStartedMs = 0;

BlinkPhase g_blinkPhase = BlinkPhase::Idle;
float g_openAmount = 1.0f;
uint32_t g_phaseStartedMs = 0;
uint32_t g_phaseDurationMs = 0;
uint32_t g_nextBlinkMs = 0;
uint32_t g_lastDrawMs = 0;
bool g_forceRedraw = false;
uint8_t g_blinksInSequence = 1;
uint8_t g_blinksDone = 0;

SleepEyeAnim g_sleepEyeAnim = SleepEyeAnim::None;
uint32_t g_sleepAnimStartedMs = 0;
uint32_t g_sleepAnimDurationMs = 0;
float g_sleepAnimFromAmount = 1.0f;

// Idle gaze
int16_t g_gazeFromX = 0;
int16_t g_gazeFromY = 0;
int16_t g_gazeX = 0;
int16_t g_gazeY = 0;
int16_t g_targetGazeX = 0;
int16_t g_targetGazeY = 0;
uint32_t g_gazeMoveStartMs = 0;
uint32_t g_gazeMoveDurationMs = 0;
uint32_t g_nextGazeShiftMs = 0;

// Typing
bool g_typingLookRight = true;
uint32_t g_typingNextFlipMs = 0;

// Reading
ReadingScanPhase g_readScanPhase = ReadingScanPhase::Forward;
int16_t g_readScanX = -5;
uint32_t g_readScanStartMs = 0;
uint32_t g_readScanDurationMs = 0;

// Thinking
int8_t g_thinkSideX = 3;
bool g_thinkSquintLeft = true;
uint32_t g_nextThinkSquintFlipMs = 0;
uint32_t g_nextThinkIdeaMs = 0;
uint32_t g_thinkIdeaUntilMs = 0;

// Impact overlay
bool g_impactActive = false;
uint32_t g_impactStartMs = 0;
uint32_t g_impactDurationMs = 0;
int8_t g_impactJitterX = 0;
int8_t g_impactJitterY = 0;

Eye eyeWithHeight(int16_t x, int16_t width, int16_t height) {
  return {
    x,
    (int16_t)(EYE_CENTER_Y - height / 2),
    width,
    height
  };
}

Eye renderEye(const Eye& base, float openAmount) {
  const int16_t renderHeight =
    (int16_t)(base.height * openAmount);

  if (renderHeight <= 0) {
    return {base.x, base.y, base.width, 0};
  }

  return {
    base.x,
    (int16_t)(base.y + (base.height - renderHeight) / 2),
    base.width,
    renderHeight
  };
}

float phaseProgress(uint32_t now, uint32_t durationMs) {
  if (durationMs == 0) {
    return 1.0f;
  }

  const uint32_t elapsed = now - g_phaseStartedMs;

  if (elapsed >= durationMs) {
    return 1.0f;
  }

  return (float)elapsed / (float)durationMs;
}

float moveProgress(uint32_t now, uint32_t startMs, uint32_t durationMs) {
  if (durationMs == 0) {
    return 1.0f;
  }

  if (now < startMs) {
    return 0.0f;
  }

  const uint32_t elapsed = now - startMs;

  if (elapsed >= durationMs) {
    return 1.0f;
  }

  return (float)elapsed / (float)durationMs;
}

int16_t lerpInt(int16_t from, int16_t to, float t) {
  return (int16_t)(from + (to - from) * t);
}

void scheduleNextBlink(uint32_t now) {
  switch (g_eyeMode) {
    case EyeMode::Typing:
      g_nextBlinkMs = now + anim::randRangeMs(1800, 4000);
      break;
    default:
      g_nextBlinkMs = now + anim::randRangeMs(2500, 5500);
      break;
  }
}

void beginClosing(uint32_t now) {
  g_blinkPhase = BlinkPhase::Closing;
  g_phaseStartedMs = now;
  g_phaseDurationMs = anim::randRangeMs(65, 120);
  g_forceRedraw = true;
}

void beginClosed(uint32_t now) {
  g_blinkPhase = BlinkPhase::Closed;
  g_phaseStartedMs = now;
  g_phaseDurationMs = anim::randRangeMs(35, 90);
  g_forceRedraw = true;
}

void beginOpening(uint32_t now) {
  g_blinkPhase = BlinkPhase::Opening;
  g_phaseStartedMs = now;
  g_phaseDurationMs = anim::randRangeMs(75, 130);
  g_forceRedraw = true;
}

void beginBlinkIdle(uint32_t now) {
  g_blinkPhase = BlinkPhase::Idle;
  g_phaseStartedMs = now;
  g_forceRedraw = true;
}

void startBlinkSequence(uint32_t now) {
  g_blinksInSequence =
    anim::randChance(DOUBLE_BLINK_CHANCE_PERCENT) ? 2 : 1;
  g_blinksDone = 0;
  beginClosing(now);
}

void finishBlinkSequence(uint32_t now) {
  g_blinksDone = 0;
  g_blinksInSequence = 1;
  beginBlinkIdle(now);
  scheduleNextBlink(now);
}

void resetIdleGaze(uint32_t now) {
  g_gazeX = 0;
  g_gazeY = 0;
  g_targetGazeX = 0;
  g_targetGazeY = 0;
  g_gazeMoveStartMs = now;
  g_gazeMoveDurationMs = 0;
  g_nextGazeShiftMs = now + anim::randRangeMs(4000, 8000);
}

void resetTypingState(uint32_t now) {
  g_typingLookRight = anim::randChance(50);
  g_typingNextFlipMs = now + anim::randRangeMs(120, 260);
}

void resetReadingState(uint32_t now) {
  g_readScanPhase = ReadingScanPhase::Forward;
  g_readScanX = -5;
  g_readScanStartMs = now;
  g_readScanDurationMs = anim::randRangeMs(1100, 1800);
}

void resetThinkingState(uint32_t now) {
  g_thinkSideX = anim::randChance(50) ? 3 : -3;
  g_thinkSquintLeft = anim::randChance(50);
  g_nextThinkSquintFlipMs = now + anim::randRangeMs(2000, 4500);
  g_nextThinkIdeaMs = now + anim::randRangeMs(5000, 10000);
  g_thinkIdeaUntilMs = 0;
}

void resetImpactState() {
  g_impactActive = false;
  g_impactStartMs = 0;
  g_impactDurationMs = 0;
  g_impactJitterX = 0;
  g_impactJitterY = 0;
}

void applyImpactOverlay(Eye& left, Eye& right, uint32_t now) {
  if (!g_impactActive) {
    return;
  }

  const float t = moveProgress(now, g_impactStartMs, g_impactDurationMs);

  if (t >= 1.0f) {
    g_impactActive = false;
    return;
  }

  left = eyeWithHeight(
    (int16_t)(left.x + g_impactJitterX),
    left.width,
    18
  );
  right = eyeWithHeight(
    (int16_t)(right.x + g_impactJitterX),
    right.width,
    18
  );
  left.y = (int16_t)(left.y + g_impactJitterY);
  right.y = (int16_t)(right.y + g_impactJitterY);
}

void updateIdleEyes(uint32_t now) {
  if (now >= g_nextGazeShiftMs) {
    g_gazeFromX = g_gazeX;
    g_gazeFromY = g_gazeY;
    g_targetGazeX = (int16_t)anim::randRangeMs(0, 4) - 2;
    g_targetGazeY = (int16_t)anim::randRangeMs(0, 2) - 1;
    g_gazeMoveStartMs = now;
    g_gazeMoveDurationMs = anim::randRangeMs(300, 400);
    g_nextGazeShiftMs = now + anim::randRangeMs(4000, 8000);
  }

  const float t = anim::easeInOutCubic(
    moveProgress(now, g_gazeMoveStartMs, g_gazeMoveDurationMs)
  );
  g_gazeX = lerpInt(g_gazeFromX, g_targetGazeX, t);
  g_gazeY = lerpInt(g_gazeFromY, g_targetGazeY, t);

  g_leftEye = {
    (int16_t)(DEFAULT_LEFT.x + g_gazeX),
    (int16_t)(DEFAULT_LEFT.y + g_gazeY),
    DEFAULT_LEFT.width,
    DEFAULT_LEFT.height
  };
  g_rightEye = {
    (int16_t)(DEFAULT_RIGHT.x + g_gazeX),
    (int16_t)(DEFAULT_RIGHT.y + g_gazeY),
    DEFAULT_RIGHT.width,
    DEFAULT_RIGHT.height
  };
}

void updateTypingEyes(uint32_t now) {
  if (now >= g_typingNextFlipMs) {
    g_typingLookRight = !g_typingLookRight;
    g_typingNextFlipMs = now + anim::randRangeMs(120, 260);
  }

  const int16_t gx = g_typingLookRight ? 2 : -2;

  g_leftEye = eyeWithHeight(
    (int16_t)(DEFAULT_LEFT.x + gx),
    DEFAULT_LEFT.width,
    12
  );
  g_rightEye = eyeWithHeight(
    (int16_t)(DEFAULT_RIGHT.x + gx),
    DEFAULT_RIGHT.width,
    12
  );
  g_leftEye.y += 2;
  g_rightEye.y += 2;
}

void updateReadingEyes(uint32_t now) {
  const float t = anim::easeInOutCubic(
    moveProgress(now, g_readScanStartMs, g_readScanDurationMs)
  );

  if (g_readScanPhase == ReadingScanPhase::Forward) {
    g_readScanX = lerpInt(-5, 5, t);

    if (t >= 1.0f) {
      g_readScanPhase = ReadingScanPhase::Return;
      g_readScanStartMs = now;
      g_readScanDurationMs = anim::randRangeMs(120, 220);
    }
  } else {
    g_readScanX = lerpInt(5, -5, t);

    if (t >= 1.0f) {
      g_readScanPhase = ReadingScanPhase::Forward;
      g_readScanStartMs = now;
      g_readScanDurationMs = anim::randRangeMs(1100, 1800);

      if (g_blinkPhase == BlinkPhase::Idle) {
        g_nextBlinkMs = now + anim::randRangeMs(80, 180);
      }
    }
  }

  g_leftEye = eyeWithHeight(
    (int16_t)(DEFAULT_LEFT.x + g_readScanX),
    DEFAULT_LEFT.width,
    DEFAULT_LEFT.height
  );
  g_rightEye = eyeWithHeight(
    (int16_t)(DEFAULT_RIGHT.x + g_readScanX),
    DEFAULT_RIGHT.width,
    DEFAULT_RIGHT.height
  );
  g_leftEye.y += 1;
  g_rightEye.y += 1;
}

void updateThinkingEyes(uint32_t now) {
  if (now >= g_nextThinkSquintFlipMs) {
    g_thinkSquintLeft = !g_thinkSquintLeft;
    g_thinkSideX = (int8_t)(g_thinkSideX * -1);
    g_nextThinkSquintFlipMs = now + anim::randRangeMs(2000, 4500);
  }

  if (now >= g_nextThinkIdeaMs && g_thinkIdeaUntilMs == 0) {
    g_thinkIdeaUntilMs = now + 180;
    g_nextThinkIdeaMs = now + anim::randRangeMs(5000, 10000);
    g_forceRedraw = true;
  }

  const bool ideaActive =
    g_thinkIdeaUntilMs != 0 && now < g_thinkIdeaUntilMs;

  if (!ideaActive && g_thinkIdeaUntilMs != 0 &&
      now >= g_thinkIdeaUntilMs) {
    g_thinkIdeaUntilMs = 0;
  }

  const int16_t leftHeight = ideaActive
    ? 17
    : (g_thinkSquintLeft ? 11 : 14);
  const int16_t rightHeight = ideaActive
    ? 17
    : (g_thinkSquintLeft ? 14 : 11);

  g_leftEye = eyeWithHeight(
    (int16_t)(DEFAULT_LEFT.x + g_thinkSideX),
    DEFAULT_LEFT.width,
    leftHeight
  );
  g_rightEye = eyeWithHeight(
    (int16_t)(DEFAULT_RIGHT.x + g_thinkSideX),
    DEFAULT_RIGHT.width,
    rightHeight
  );
  g_leftEye.y -= 2;
  g_rightEye.y -= 2;
}

void updateRingEyes(uint32_t now) {
  (void)now;

  g_leftEye = eyeWithHeight(DEFAULT_LEFT.x, DEFAULT_LEFT.width, 9);
  g_rightEye = eyeWithHeight(DEFAULT_RIGHT.x, DEFAULT_RIGHT.width, 9);
}

void updateWelcomeEyes(uint32_t now) {
  int16_t leftHeight = 14;
  int16_t rightHeight = 14;
  int16_t yOffset = 0;

  if (!welcomeAudioStarted()) {
    leftHeight = 16;
    rightHeight = 16;
    yOffset = -2;
  } else {
    const uint32_t audioElapsed = welcomeAudioElapsed(now);

    if (audioElapsed < WELCOME_AUDIO_WELCOME_END_MS) {
      leftHeight = 16;
      rightHeight = 16;
      yOffset = -2;
    } else if (audioElapsed < WELCOME_AUDIO_PAUSE_END_MS) {
      leftHeight = 16;
      rightHeight = 16;
      yOffset = -2;

      if (audioElapsed >= 870 && audioElapsed < 970) {
        const float blinkT = audioElapsed < 920
          ? (float)(audioElapsed - 870) / 50.0f
          : (float)(970 - audioElapsed) / 50.0f;
        const float open = anim::easeInOutCubic(
          constrain(blinkT, 0.0f, 1.0f)
        );
        leftHeight = (int16_t)(2.0f + 14.0f * open);
        rightHeight = leftHeight;
      }
    } else if (audioElapsed < WELCOME_AUDIO_LOGIN_END_MS) {
      leftHeight = 12;
      rightHeight = 12;
    } else if (audioElapsed < WELCOME_AUDIO_ACCEPTED_END_MS) {
      leftHeight = 17;
      rightHeight = 15;
    }
  }

  g_leftEye = eyeWithHeight(
    DEFAULT_LEFT.x,
    DEFAULT_LEFT.width,
    leftHeight
  );
  g_rightEye = eyeWithHeight(
    DEFAULT_RIGHT.x,
    DEFAULT_RIGHT.width,
    rightHeight
  );
  g_leftEye.y += yOffset;
  g_rightEye.y += yOffset;
}

void updateAttentionEyes(uint32_t now) {
  const uint32_t elapsed = now - g_modeStartedMs;
  const bool opening = elapsed < 2200;
  const int16_t height = opening ? 16 : 15;
  int16_t yOffset = opening ? -2 : -1;

  if (!opening && ((elapsed / 1400) % 2u) == 0u) {
    yOffset -= 1;
  }

  g_leftEye = eyeWithHeight(
    DEFAULT_LEFT.x,
    DEFAULT_LEFT.width,
    height
  );
  g_rightEye = eyeWithHeight(
    DEFAULT_RIGHT.x,
    DEFAULT_RIGHT.width,
    height
  );
  g_leftEye.y += yOffset;
  g_rightEye.y += yOffset;
}

void updateModePose(uint32_t now) {
  switch (g_eyeMode) {
    case EyeMode::Idle:
      updateIdleEyes(now);
      break;
    case EyeMode::Typing:
      updateTypingEyes(now);
      break;
    case EyeMode::Reading:
      updateReadingEyes(now);
      break;
    case EyeMode::Thinking:
      updateThinkingEyes(now);
      break;
    case EyeMode::Ring:
      updateRingEyes(now);
      break;
    case EyeMode::Welcome:
      updateWelcomeEyes(now);
      break;
    case EyeMode::Attention:
      updateAttentionEyes(now);
      break;
  }

  applyImpactOverlay(g_leftEye, g_rightEye, now);
}

void advanceBlinkPhase(uint32_t now) {
  switch (g_blinkPhase) {
    case BlinkPhase::Idle:
      if (now >= g_nextBlinkMs) {
        startBlinkSequence(now);
      }
      break;

    case BlinkPhase::Closing: {
      const float t = anim::easeInOutCubic(
        phaseProgress(now, g_phaseDurationMs)
      );
      g_openAmount = 1.0f - t * (1.0f - BLINK_CLOSED_AMOUNT);

      if (t >= 1.0f) {
        g_openAmount = BLINK_CLOSED_AMOUNT;
        beginClosed(now);
      }
      break;
    }

    case BlinkPhase::Closed:
      if (now - g_phaseStartedMs >= g_phaseDurationMs) {
        beginOpening(now);
      }
      break;

    case BlinkPhase::Opening: {
      const float t = anim::easeInOutCubic(
        phaseProgress(now, g_phaseDurationMs)
      );
      g_openAmount =
        BLINK_CLOSED_AMOUNT + t * (1.0f - BLINK_CLOSED_AMOUNT);

      if (t >= 1.0f) {
        g_openAmount = 1.0f;
        g_blinksDone++;

        if (g_blinksDone < g_blinksInSequence) {
          beginBlinkIdle(now);
          g_nextBlinkMs = now + anim::randRangeMs(90, 220);
        } else {
          finishBlinkSequence(now);
        }
      }
      break;
    }
  }
}

void drawCurrentEyes() {
  const Eye left = renderEye(g_leftEye, g_openAmount);
  const Eye right = renderEye(g_rightEye, g_openAmount);
  drawEyes(left, right, EYE_CORNER_RADIUS);
}

bool advanceSleepEyeAnim(uint32_t now) {
  if (g_sleepEyeAnim == SleepEyeAnim::None) {
    return false;
  }

  const float t = anim::easeInOutCubic(
    moveProgress(now, g_sleepAnimStartedMs, g_sleepAnimDurationMs)
  );

  if (g_sleepEyeAnim == SleepEyeAnim::Closing) {
    g_openAmount = g_sleepAnimFromAmount * (1.0f - t);

    if (t >= 1.0f) {
      g_openAmount = 0.0f;
      g_sleepEyeAnim = SleepEyeAnim::None;
      return true;
    }
  } else {
    g_openAmount = g_sleepAnimFromAmount + t * (1.0f - g_sleepAnimFromAmount);

    if (t >= 1.0f) {
      g_openAmount = 1.0f;
      g_sleepEyeAnim = SleepEyeAnim::None;
      beginBlinkIdle(now);
      scheduleNextBlink(now);
      return true;
    }
  }

  return false;
}

}  // namespace

void setEyeMode(EyeMode mode, uint32_t now) {
  g_eyeMode = mode;
  g_modeStartedMs = now;
  resetImpactState();

  switch (mode) {
    case EyeMode::Idle:
      resetIdleGaze(now);
      break;
    case EyeMode::Typing:
      resetTypingState(now);
      break;
    case EyeMode::Reading:
      resetReadingState(now);
      break;
    case EyeMode::Thinking:
      resetThinkingState(now);
      break;
    case EyeMode::Ring:
      break;
    case EyeMode::Welcome:
      g_blinkPhase = BlinkPhase::Idle;
      g_openAmount = 1.0f;
      g_nextBlinkMs = now + 10000;
      break;
    case EyeMode::Attention:
      g_blinkPhase = BlinkPhase::Idle;
      g_openAmount = 1.0f;
      g_nextBlinkMs = now + 2500;
      break;
  }

  updateModePose(now);
  g_forceRedraw = true;
}

void triggerEyeImpact(uint32_t now) {
  g_impactActive = true;
  g_impactStartMs = now;
  g_impactDurationMs = anim::randRangeMs(180, 260);
  g_impactJitterX = (int8_t)(anim::randRangeMs(0, 4) - 2);
  g_impactJitterY = (int8_t)(anim::randRangeMs(0, 4) - 2);
  g_forceRedraw = true;
}

void startEyes() {
  g_eyesActive = true;
  g_blinksInSequence = 1;
  g_blinksDone = 0;
  g_openAmount = 1.0f;
  g_lastDrawMs = 0;
  g_forceRedraw = true;
  beginBlinkIdle(millis());
  g_nextBlinkMs = millis() + anim::randRangeMs(800, 2000);
  setEyeMode(EyeMode::Idle, millis());
  drawCurrentEyes();
  g_lastDrawMs = millis();
  g_forceRedraw = false;
}

void stopEyes() {
  g_eyesActive = false;
  g_sleepEyeAnim = SleepEyeAnim::None;
}

void requestSleepEyeClose(uint32_t now) {
  if (!g_eyesActive) {
    startEyes();
  }

  g_sleepEyeAnim = SleepEyeAnim::Closing;
  g_sleepAnimFromAmount = g_openAmount;
  g_sleepAnimStartedMs = now;
  g_sleepAnimDurationMs = 280;
  g_forceRedraw = true;
}

void requestSleepEyeOpen(uint32_t now) {
  g_sleepEyeAnim = SleepEyeAnim::Opening;
  g_sleepAnimFromAmount = g_openAmount;
  g_sleepAnimStartedMs = now;
  g_sleepAnimDurationMs = 280;
  g_forceRedraw = true;
}

void startEyesForWake(uint32_t now) {
  g_eyesActive = true;
  g_blinksInSequence = 1;
  g_blinksDone = 0;
  g_openAmount = 0.0f;
  g_lastDrawMs = 0;
  g_forceRedraw = true;
  g_sleepEyeAnim = SleepEyeAnim::None;
  setEyeMode(EyeMode::Idle, now);
  requestSleepEyeOpen(now);
}

SleepEyeResult updateSleepEyes(uint32_t now) {
  if (g_sleepEyeAnim == SleepEyeAnim::None) {
    return SleepEyeResult::Running;
  }

  updateModePose(now);

  const SleepEyeAnim phase = g_sleepEyeAnim;
  const bool finished = advanceSleepEyeAnim(now);

  const bool shouldDraw = g_forceRedraw ||
    finished ||
    (now - g_lastDrawMs) >= REDRAW_INTERVAL_MS;

  if (shouldDraw) {
    drawCurrentEyes();
    g_lastDrawMs = now;
    g_forceRedraw = false;
  }

  if (!finished) {
    return SleepEyeResult::Running;
  }

  if (phase == SleepEyeAnim::Closing) {
    return SleepEyeResult::CloseComplete;
  }

  return SleepEyeResult::OpenComplete;
}

void updateEyes(uint32_t now) {
  if (!g_eyesActive) {
    return;
  }

  if (g_sleepEyeAnim != SleepEyeAnim::None) {
    return;
  }

  updateModePose(now);

  if (g_eyeMode != EyeMode::Welcome) {
    advanceBlinkPhase(now);
  }

  if (!g_forceRedraw &&
      (now - g_lastDrawMs) < REDRAW_INTERVAL_MS) {
    return;
  }

  drawCurrentEyes();
  g_lastDrawMs = now;
  g_forceRedraw = false;
}

bool eyesActive() {
  return g_eyesActive;
}

const Eye& leftEye() {
  return g_leftEye;
}

const Eye& rightEye() {
  return g_rightEye;
}

Eye& mutableLeftEye() {
  return g_leftEye;
}

Eye& mutableRightEye() {
  return g_rightEye;
}
