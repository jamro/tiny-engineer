#include "display/eyes/styles/eye_style.h"

#include <Adafruit_SSD1306.h>

#include "animation/dead.h"
#include "animation/util.h"
#include "audio/audio_motion.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/styles/kawaii/face.h"
#include "display/eyes/styles/kawaii/mood.h"
#include "display/oled.h"
#include "display/oled_internal.h"

namespace {

// A longer gap means the style was just selected or the eyes restarted:
// jump straight to the mood instead of easing from a stale face.
constexpr uint32_t kSnapAfterGapMs = 250;

uint8_t g_frame[kawaii::kFrameBytes];
kawaii::Animator g_animator;
kawaii::IdleDirector g_idle(anim::randRangeMs);
uint32_t g_lastFrameMs = 0;
bool g_armed = false;
bool g_wasIdle = false;

kawaii::Mood modeMood(EyeMode mode) {
  switch (mode) {
    case EyeMode::Idle:
      return kawaii::Mood::Content;
    case EyeMode::Typing:
      return kawaii::Mood::Focused;
    case EyeMode::Reading:
      return kawaii::Mood::Reading;
    case EyeMode::Thinking:
      return kawaii::Mood::Thinking;
    case EyeMode::Ring:
      return kawaii::Mood::Surprised;
    case EyeMode::Welcome:
      return kawaii::Mood::Excited;
    case EyeMode::Attention:
      return kawaii::Mood::Curious;
    case EyeMode::Error:
      return kawaii::Mood::Angry;
    case EyeMode::Abort:
      return kawaii::Mood::Panic;
    case EyeMode::Wakeup:
      return kawaii::Mood::Sleepy;
    case EyeMode::Dead:
      return deadShowingX() ? kawaii::Mood::Dizzy : kawaii::Mood::Sad;
    case EyeMode::Talking:
      return kawaii::Mood::Talking;
  }

  return kawaii::Mood::Content;
}

kawaii::EyePose eyePose(const Eye& eye) {
  return {
    eye.x + eye.width * 0.5f,
    eye.y + eye.height * 0.5f,
    (float)eye.height / (float)eyes::DEFAULT_LEFT.height
  };
}

void drawKawaii(
  EyeMode mode,
  AnimationId animation,
  uint32_t /*modeStartedMs*/,
  uint32_t now,
  EyeStyleSleepPhase sleepPhase
) {
  if (!oledAvailable) {
    return;
  }

  const bool sleeping = sleepPhase == EyeStyleSleepPhase::Closing ||
    animation == AnimationId::Sleep;
  const bool idle = !sleeping && mode == EyeMode::Idle;

  if (idle && !g_wasIdle) {
    g_idle.reset(now);
  }
  g_wasIdle = idle;

  kawaii::Mood mood = kawaii::Mood::Asleep;
  if (!sleeping) {
    mood = idle ? g_idle.update(now) : modeMood(mode);
  }

  kawaii::Expression target = kawaii::expressionFor(mood);
  if (mood == kawaii::Mood::Talking) {
    const AudioMotionSnapshot speech = audioMotionSnapshot(now);
    kawaii::applySpeech(target, speech.level, speech.streamActive && speech.speaking);
  }

  const uint32_t gap = now - g_lastFrameMs;
  if (!g_armed || gap > kSnapAfterGapMs) {
    g_animator.snap(target);
    g_armed = true;
  } else {
    g_animator.update(target, gap);
  }
  g_lastFrameMs = now;

  const kawaii::Pose pose = {
    eyePose(leftEye()),
    eyePose(rightEye()),
    blinkOpenAmount()
  };

  kawaii::Face face = {};
  kawaii::composeFace(pose, g_animator, now, face);
  if (!kawaii::render(face, g_frame, sizeof(g_frame))) {
    return;
  }

  display.clearDisplay();
  display.drawBitmap(
    0,
    0,
    g_frame,
    kawaii::kWidth,
    kawaii::kHeight,
    SSD1306_WHITE
  );
  display.display();
}

}  // namespace

extern const EyeStyleRenderer kKawaiiEyeStyle = {
  "kawaii",
  true,
  true,
  drawKawaii,
};
