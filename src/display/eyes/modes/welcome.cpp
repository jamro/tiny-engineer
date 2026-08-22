#include "display/eyes/modes/welcome.h"

#include <Arduino.h>

#include "animation/util.h"
#include "animation/welcome.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

void startWelcomeEyes(uint32_t now) {
  (void)now;
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

    if (audioElapsed < WELCOME_AUDIO_GREETING_END_MS) {
      leftHeight = 16;
      rightHeight = 16;
      yOffset = -2;
    } else if (audioElapsed < WELCOME_AUDIO_PAUSE_END_MS) {
      leftHeight = 16;
      rightHeight = 16;
      yOffset = -2;

      if (audioElapsed >= WELCOME_AUDIO_BLINK_START_MS &&
          audioElapsed < WELCOME_AUDIO_BLINK_END_MS) {
        const uint32_t blinkMid =
          (WELCOME_AUDIO_BLINK_START_MS + WELCOME_AUDIO_BLINK_END_MS) / 2;
        const uint32_t halfBlink =
          (WELCOME_AUDIO_BLINK_END_MS - WELCOME_AUDIO_BLINK_START_MS) / 2;
        const float blinkT = audioElapsed < blinkMid
          ? (float)(audioElapsed - WELCOME_AUDIO_BLINK_START_MS) /
            (float)halfBlink
          : (float)(WELCOME_AUDIO_BLINK_END_MS - audioElapsed) /
            (float)halfBlink;
        const float open = anim::easeInOutCubic(
          constrain(blinkT, 0.0f, 1.0f)
        );
        leftHeight = (int16_t)(2.0f + 14.0f * open);
        rightHeight = leftHeight;
      }
    } else if (audioElapsed < WELCOME_AUDIO_QUESTION_END_MS) {
      leftHeight = 12;
      rightHeight = 12;
    } else if (audioElapsed < WELCOME_AUDIO_END_MS) {
      leftHeight = 17;
      rightHeight = 15;
    }
  }

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    eyes::DEFAULT_LEFT.x,
    eyes::DEFAULT_LEFT.width,
    leftHeight
  );
  right = eyes::eyeWithHeight(
    eyes::DEFAULT_RIGHT.x,
    eyes::DEFAULT_RIGHT.width,
    rightHeight
  );
  left.y += yOffset;
  right.y += yOffset;
}
