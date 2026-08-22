#include "display/eyes/modes/attention.h"

#include <Arduino.h>

#include "animation/attention.h"
#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

void startAttentionEyes(uint32_t now) {
  (void)now;
}

void updateAttentionEyes(uint32_t now) {
  int16_t leftHeight = 16;
  int16_t rightHeight = 16;
  int16_t yOffset = -2;

  if (attentionAudioStarted()) {
    const uint32_t audioElapsed = attentionAudioElapsed(now);

    if (audioElapsed < ATTENTION_AUDIO_END_MS) {
      leftHeight = 16;
      rightHeight = 16;
      yOffset = -2;

      if (audioElapsed >= ATTENTION_AUDIO_BLINK_START_MS &&
          audioElapsed < ATTENTION_AUDIO_BLINK_END_MS) {
        const uint32_t blinkMid =
          (ATTENTION_AUDIO_BLINK_START_MS + ATTENTION_AUDIO_BLINK_END_MS) / 2;
        const uint32_t halfBlink =
          (ATTENTION_AUDIO_BLINK_END_MS - ATTENTION_AUDIO_BLINK_START_MS) / 2;
        const float blinkT = audioElapsed < blinkMid
          ? (float)(audioElapsed - ATTENTION_AUDIO_BLINK_START_MS) /
            (float)halfBlink
          : (float)(ATTENTION_AUDIO_BLINK_END_MS - audioElapsed) /
            (float)halfBlink;
        const float open = anim::easeInOutCubic(
          constrain(blinkT, 0.0f, 1.0f)
        );
        leftHeight = (int16_t)(2.0f + 14.0f * open);
        rightHeight = leftHeight;
      } else if (audioElapsed >= ATTENTION_AUDIO_TURN_END_MS) {
        leftHeight = 17;
        rightHeight = 15;
      }
    } else {
      const uint32_t waitElapsed = audioElapsed - ATTENTION_AUDIO_END_MS;
      leftHeight = 15;
      rightHeight = 15;
      yOffset = -1;

      if ((waitElapsed / 1400) % 2u == 0u) {
        yOffset -= 1;
      }
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
