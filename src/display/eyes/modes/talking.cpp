#include "display/eyes/modes/talking.h"

#include "animation/util.h"
#include "audio/audio_motion.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

namespace {

int16_t g_glanceX = 0;
int16_t g_glanceY = 0;
uint32_t g_nextGlanceMs = 0;

}  // namespace

void startTalkingEyes(uint32_t now) {
  g_glanceX = 0;
  g_glanceY = 0;
  g_nextGlanceMs = now + anim::randRangeMs(400, 900);
}

void updateTalkingEyes(uint32_t now) {
  const AudioMotionSnapshot snap = audioMotionSnapshot(now);

  int16_t height = 15;

  if (snap.streamActive && snap.speaking) {
    // Hold gaze on the listener while speaking; bump height on emphasis.
    g_glanceX = 0;
    g_glanceY = 0;
    g_nextGlanceMs = now + anim::randRangeMs(500, 1200);
    if (snap.emphasis > 0.2f) {
      height = 16;
    }
  } else if (now >= g_nextGlanceMs) {
    // Ambient or stream silence: short glances away.
    const bool away = g_glanceX == 0 && anim::randChance(45);
    g_glanceX = away ? (int16_t)(anim::randChance(50) ? 4 : -4) : 0;
    g_glanceY = away ? (int16_t)anim::randRangeMs(0, 2) - 2 : 0;
    g_nextGlanceMs = now + (away ? anim::randRangeMs(250, 600) : anim::randRangeMs(700, 1600));
  }

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_LEFT.x + g_glanceX),
    eyes::DEFAULT_LEFT.width,
    height
  );
  right = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_RIGHT.x + g_glanceX),
    eyes::DEFAULT_RIGHT.width,
    height
  );
  left.y += g_glanceY;
  right.y += g_glanceY;
}
