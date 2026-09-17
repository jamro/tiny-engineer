#include <Arduino.h>

#include <cstring>

#include "animation.h"
#include "animation/registry.h"
#include "animation/util.h"
#include "display/eyes.h"
#include "display/oled.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/impact.h"
#include "display/eyes/core/internal.h"
#include "display/eyes/core/util.h"
#include "display/eyes/styles/eye_style.h"
#include "display/eyes/styles/kaomoji.h"
#include "settings/settings.h"

namespace {

enum class SleepEyeAnim {
  None,
  Closing,
  Opening
};

Eye g_leftEye = eyes::DEFAULT_LEFT;
Eye g_rightEye = eyes::DEFAULT_RIGHT;

bool g_eyesActive = false;
EyeMode g_eyeMode = EyeMode::Idle;
uint32_t g_modeStartedMs = 0;

uint32_t g_lastDrawMs = 0;
bool g_forceRedraw = false;

SleepEyeAnim g_sleepEyeAnim = SleepEyeAnim::None;
uint32_t g_sleepAnimStartedMs = 0;
uint32_t g_sleepAnimDurationMs = 0;
float g_sleepAnimFromAmount = 1.0f;

char g_lastDrawnStyle[SETTINGS_EYES_STYLE_MAX_LEN + 1] = {};

EyeStyleSleepPhase currentSleepPhase() {
  switch (g_sleepEyeAnim) {
    case SleepEyeAnim::Closing:
      return EyeStyleSleepPhase::Closing;
    case SleepEyeAnim::Opening:
      return EyeStyleSleepPhase::Opening;
    case SleepEyeAnim::None:
    default:
      return EyeStyleSleepPhase::None;
  }
}

void updateModePose(uint32_t now) {
  const ModeEntry* entry = modeByEyeMode(g_eyeMode);

  if (entry->updateEyes != nullptr) {
    entry->updateEyes(now);
  }

  applyImpactOverlay(g_leftEye, g_rightEye, now);
}

void drawCurrentEyes(uint32_t now) {
  const EyeStyleRenderer* style = currentEyeStyle();
  style->draw(
    g_eyeMode,
    getAnimation(),
    g_modeStartedMs,
    now,
    currentSleepPhase()
  );
}

bool advanceSleepEyeAnim(uint32_t now) {
  if (g_sleepEyeAnim == SleepEyeAnim::None) {
    return false;
  }

  const float t = anim::easeInOutCubic(
    eyes::moveProgress(now, g_sleepAnimStartedMs, g_sleepAnimDurationMs)
  );

  if (g_sleepEyeAnim == SleepEyeAnim::Closing) {
    blinkSetOpenAmount(g_sleepAnimFromAmount * (1.0f - t));

    if (t >= 1.0f) {
      blinkSetOpenAmount(0.0f);
      g_sleepEyeAnim = SleepEyeAnim::None;
      return true;
    }
  } else {
    blinkSetOpenAmount(
      g_sleepAnimFromAmount + t * (1.0f - g_sleepAnimFromAmount)
    );

    if (t >= 1.0f) {
      g_sleepEyeAnim = SleepEyeAnim::None;
      blinkOnSleepOpenComplete(now);
      return true;
    }
  }

  return false;
}

void noteStyleIfChanged() {
  const char* styleId = settingsEyesStyle();
  if (strcmp(g_lastDrawnStyle, styleId) != 0) {
    strncpy(g_lastDrawnStyle, styleId, SETTINGS_EYES_STYLE_MAX_LEN);
    g_lastDrawnStyle[SETTINGS_EYES_STYLE_MAX_LEN] = '\0';
    g_forceRedraw = true;
    if (strcmp(styleId, "kaomoji") == 0) {
      kaomojiResetPlayback(millis());
    }
  }
}

}  // namespace

namespace eyes {

uint32_t modeStartedMs() {
  return g_modeStartedMs;
}

void setModeStartedMs(uint32_t ms) {
  g_modeStartedMs = ms;
}

EyeMode currentEyeMode() {
  return g_eyeMode;
}

void setCurrentEyeMode(EyeMode mode) {
  g_eyeMode = mode;
}

bool forceRedraw() {
  return g_forceRedraw;
}

void setForceRedraw(bool value) {
  g_forceRedraw = value;
}

void requestForceRedraw() {
  g_forceRedraw = true;
}

uint32_t lastDrawMs() {
  return g_lastDrawMs;
}

void setLastDrawMs(uint32_t ms) {
  g_lastDrawMs = ms;
}

bool eyesRunning() {
  return g_eyesActive;
}

void setEyesRunning(bool active) {
  g_eyesActive = active;
}

}  // namespace eyes

void setEyeMode(EyeMode mode, uint32_t now) {
  g_eyeMode = mode;
  g_modeStartedMs = now;
  resetImpactState();

  const ModeEntry* entry = modeByEyeMode(mode);

  if (entry->blinkHoldMs >= 0) {
    blinkBeginIdle(now);
    blinkSetOpenAmount(1.0f);
    blinkSetNextBlinkMs(now + static_cast<uint32_t>(entry->blinkHoldMs));
  }

  if (entry->startEyes != nullptr) {
    entry->startEyes(now);
  }

  if (currentEyeStyle()->needsPoseUpdate) {
    updateModePose(now);
  }

  g_forceRedraw = true;
}

void startEyes() {
  g_eyesActive = true;
  blinkResetCounters();
  blinkSetOpenAmount(1.0f);
  g_lastDrawMs = 0;
  g_forceRedraw = true;
  blinkBeginIdle(millis());
  blinkSetNextBlinkMs(millis() + anim::randRangeMs(800, 2000));
  setEyeMode(EyeMode::Idle, millis());
  drawCurrentEyes(millis());
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
  g_sleepAnimFromAmount = blinkOpenAmount();
  g_sleepAnimStartedMs = now;
  g_sleepAnimDurationMs = 280;
  g_forceRedraw = true;
}

void requestSleepEyeOpen(uint32_t now) {
  g_sleepEyeAnim = SleepEyeAnim::Opening;
  g_sleepAnimFromAmount = blinkOpenAmount();
  g_sleepAnimStartedMs = now;
  g_sleepAnimDurationMs = 280;
  g_forceRedraw = true;
}

void clearSleepEyeAnim() {
  g_sleepEyeAnim = SleepEyeAnim::None;
}

void startEyesForWake(uint32_t now) {
  g_eyesActive = true;
  blinkResetCounters();
  blinkSetOpenAmount(0.0f);
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

  noteStyleIfChanged();

  const EyeStyleRenderer* style = currentEyeStyle();
  if (style->needsPoseUpdate) {
    updateModePose(now);
  }

  const SleepEyeAnim phase = g_sleepEyeAnim;
  const bool finished = advanceSleepEyeAnim(now);

  const bool shouldDraw = g_forceRedraw ||
    finished ||
    (now - g_lastDrawMs) >= eyes::REDRAW_INTERVAL_MS;

  if (shouldDraw) {
    drawCurrentEyes(now);
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

  noteStyleIfChanged();

  const EyeStyleRenderer* style = currentEyeStyle();

  if (style->needsPoseUpdate) {
    updateModePose(now);
  }

  if (style->needsBlink &&
      g_eyeMode != EyeMode::Welcome &&
      g_eyeMode != EyeMode::Wakeup &&
      g_eyeMode != EyeMode::Dead) {
    blinkAdvance(now);
  }

  const uint32_t redrawInterval = style->needsPoseUpdate
    ? eyes::REDRAW_INTERVAL_MS
    : 100;

  if (!g_forceRedraw && (now - g_lastDrawMs) < redrawInterval) {
    return;
  }

  drawCurrentEyes(now);
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
