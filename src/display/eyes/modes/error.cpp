#include "display/eyes/modes/error.h"

#include "animation/error.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

void startErrorEyes(uint32_t now) {
  (void)now;
}

void updateErrorEyes(uint32_t now) {
  int16_t xOffset = -4;
  int16_t leftHeight = 11;
  int16_t rightHeight = 15;

  if (errorAudioStarted()) {
    const uint32_t audioElapsed = errorAudioElapsed(now);

    if (audioElapsed < ERROR_AUDIO_END_MS) {
      if (audioElapsed < ERROR_AUDIO_UHOH_END_MS) {
        xOffset = -4;
        leftHeight = 11;
        rightHeight = 15;
      } else if (audioElapsed < ERROR_AUDIO_HUMAN_END_MS) {
        xOffset = 0;
        leftHeight = 13;
        rightHeight = 14;
      } else if (audioElapsed < ERROR_AUDIO_PROBLEM_END_MS) {
        xOffset = -4;
        leftHeight = 11;
        rightHeight = 15;
      } else {
        xOffset = -2;
        leftHeight = 12;
        rightHeight = 14;
      }
    } else {
      const uint32_t holdElapsed = audioElapsed - ERROR_AUDIO_END_MS;
      const bool glanceUser = (holdElapsed / 1800) % 2u == 1u;
      xOffset = glanceUser ? 0 : -4;
      leftHeight = glanceUser ? 13 : 11;
      rightHeight = glanceUser ? 14 : 15;
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
