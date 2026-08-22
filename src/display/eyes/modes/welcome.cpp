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
