#include "display/eyes/modes/error.h"

#include "animation/error.h"
#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

namespace {

enum class ErrorEyeLook {
  Task,
  Human,
  Away,
};

uint32_t g_nextErrorEyeScanMs = 0;
ErrorEyeLook g_errorEyeLook = ErrorEyeLook::Task;
bool g_errorEyeTense = true;

void scheduleNextErrorEyeScan(uint32_t now) {
  g_nextErrorEyeScanMs = now + anim::randRangeMs(360, 820);
}

void advanceErrorEyeScan(uint32_t now) {
  if (now < g_nextErrorEyeScanMs) {
    return;
  }

  switch (g_errorEyeLook) {
    case ErrorEyeLook::Task:
      g_errorEyeLook = ErrorEyeLook::Human;
      break;
    case ErrorEyeLook::Human:
      g_errorEyeLook = ErrorEyeLook::Away;
      break;
    case ErrorEyeLook::Away:
    default:
      g_errorEyeLook = ErrorEyeLook::Task;
      break;
  }

  g_errorEyeTense = !g_errorEyeTense;
  scheduleNextErrorEyeScan(now);
}

void applyErrorEyeLook(
  ErrorEyeLook look,
  bool tense,
  int16_t& xOffset,
  int16_t& leftHeight,
  int16_t& rightHeight
) {
  switch (look) {
    case ErrorEyeLook::Task:
      xOffset = -5;
      leftHeight = tense ? 10 : 11;
      rightHeight = 15;
      break;
    case ErrorEyeLook::Human:
      xOffset = 5;
      leftHeight = tense ? 12 : 13;
      rightHeight = tense ? 13 : 14;
      break;
    case ErrorEyeLook::Away:
    default:
      xOffset = -1;
      leftHeight = tense ? 11 : 12;
      rightHeight = tense ? 14 : 15;
      break;
  }
}

}  // namespace

void startErrorEyes(uint32_t now) {
  g_nextErrorEyeScanMs = now + ERROR_AUDIO_UHOH_END_MS;
  g_errorEyeLook = ErrorEyeLook::Task;
  g_errorEyeTense = true;
}

void updateErrorEyes(uint32_t now) {
  int16_t xOffset = -4;
  int16_t leftHeight = 11;
  int16_t rightHeight = 15;

  if (errorAudioStarted()) {
    const uint32_t audioElapsed = errorAudioElapsed(now);

    if (audioElapsed < ERROR_AUDIO_END_MS) {
      if (audioElapsed < ERROR_AUDIO_UHOH_END_MS) {
        applyErrorEyeLook(
          ErrorEyeLook::Task,
          true,
          xOffset,
          leftHeight,
          rightHeight
        );
      } else if (audioElapsed < ERROR_AUDIO_HUMAN_END_MS) {
        applyErrorEyeLook(
          ErrorEyeLook::Human,
          false,
          xOffset,
          leftHeight,
          rightHeight
        );
      } else if (audioElapsed < ERROR_AUDIO_PROBLEM_END_MS) {
        const uint32_t scanElapsed =
          audioElapsed - ERROR_AUDIO_HUMAN_END_MS;
        const uint32_t scanStep = (scanElapsed / 360) % 3u;
        const ErrorEyeLook scanLook = scanStep == 0u
          ? ErrorEyeLook::Task
          : (scanStep == 1u ? ErrorEyeLook::Human : ErrorEyeLook::Away);
        applyErrorEyeLook(
          scanLook,
          scanStep != 1u,
          xOffset,
          leftHeight,
          rightHeight
        );
      } else {
        applyErrorEyeLook(
          ErrorEyeLook::Away,
          true,
          xOffset,
          leftHeight,
          rightHeight
        );
      }
    } else {
      advanceErrorEyeScan(now);
      applyErrorEyeLook(
        g_errorEyeLook,
        g_errorEyeTense,
        xOffset,
        leftHeight,
        rightHeight
      );
    }
  }

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_LEFT.x + xOffset),
    eyes::DEFAULT_LEFT.width,
    leftHeight
  );
  right = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_RIGHT.x + xOffset),
    eyes::DEFAULT_RIGHT.width,
    rightHeight
  );
  left.y += 1;
  right.y -= 1;
}
