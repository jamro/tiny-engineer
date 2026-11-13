#include "display/eyes/styles/eye_style.h"

#include <Adafruit_SSD1306.h>

#include "animation/dead.h"
#include "animation/util.h"
#include "audio/audio_motion.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/styles/kawaii/animator.h"
#include "display/eyes/styles/kawaii/compose.h"
#include "display/eyes/styles/kawaii/face.h"
#include "display/eyes/styles/kawaii/idle.h"
#include "display/eyes/styles/kawaii/speech.h"
#include "display/oled.h"
#include "display/oled_internal.h"

// Firmware glue for the kawaii style: picks the mood for the current
// animation, feeds speech, and draws the frame. The face itself lives in
// kawaii/ (see kawaii/README.md).

namespace {

// A longer gap means the style was just selected or the eyes restarted:
// jump straight to the mood instead of easing from a stale face.
constexpr uint32_t kSnapAfterGapMs = 250;

// Eye-mode glances that map to a full look to one side (error and attention
// look ±5 px at the task or the human; thinking looks 2-3 px up).
constexpr float kGlanceRangeXPx = 5.0f;
constexpr float kGlanceRangeYPx = 3.0f;

uint8_t g_frame[kawaii::kFrameBytes];
kawaii::Animator g_animator;
kawaii::IdleDirector g_idle(anim::randRangeMs);
kawaii::Babbler g_babbler(anim::randRangeMs);
kawaii::Mood g_lastMood = kawaii::Mood::Count;
uint32_t g_lastFrameMs = 0;
bool g_armed = false;

kawaii::Mood modeMood(EyeMode mode) {
  switch (mode) {
    case EyeMode::Idle:
      return kawaii::Mood::Content;  // currentMood() asks the idle director
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

kawaii::Mood currentMood(EyeMode mode, AnimationId animation, uint32_t now, EyeStyleSleepPhase sleepPhase) {
  const bool sleeping = sleepPhase == EyeStyleSleepPhase::Closing || animation == AnimationId::Sleep;
  if (sleeping) {
    return kawaii::Mood::Asleep;
  }
  if (mode != EyeMode::Idle) {
    return modeMood(mode);
  }

  if (!kawaii::isIdleMood(g_lastMood)) {
    g_idle.reset(now);  // idle just began: start from a smile
  }
  return g_idle.update(now);
}

// Talking: opens the mouth per syllable, from /play audio when it streams.
kawaii::Voice currentVoice(uint32_t now) {
  if (g_lastMood != kawaii::Mood::Talking) {
    g_babbler.reset(now);
  }

  const AudioMotionSnapshot speech = audioMotionSnapshot(now);
  if (!speech.streamActive) {
    return g_babbler.babble(now);
  }
  return g_babbler.follow(now, speech.level, speech.emphasis, speech.speaking);
}

void stepAnimator(const kawaii::Expression& target, uint32_t now) {
  const uint32_t gap = now - g_lastFrameMs;
  if (!g_armed || gap > kSnapAfterGapMs) {
    g_animator.snap(target);
    g_armed = true;
  } else {
    g_animator.update(target, gap);
  }
  g_lastFrameMs = now;
}

kawaii::EyePose eyePose(const Eye& eye) {
  return {
    eye.x + eye.width * 0.5f,
    eye.y + eye.height * 0.5f,
    (float)eye.height / (float)eyes::DEFAULT_LEFT.height
  };
}

// The pupils look where the shared choreography moved the eyes. Centers use
// integer halves, so a height change alone does not read as a glance.
float glance(int16_t pos, int16_t size, int16_t restPos, int16_t restSize, float range) {
  return (float)((pos + size / 2) - (restPos + restSize / 2)) / range;
}

void drawFrame(uint32_t now) {
  const Eye& left = leftEye();
  const kawaii::Pose pose = {
    eyePose(left),
    eyePose(rightEye()),
    blinkOpenAmount(),
    glance(left.x, left.width, eyes::DEFAULT_LEFT.x, eyes::DEFAULT_LEFT.width, kGlanceRangeXPx),
    glance(left.y, left.height, eyes::DEFAULT_LEFT.y, eyes::DEFAULT_LEFT.height, kGlanceRangeYPx),
  };
  kawaii::Face face = {};
  kawaii::composeFace(pose, g_animator, now, face);
  if (!kawaii::render(face, g_frame, sizeof(g_frame))) {
    return;
  }

  display.clearDisplay();
  display.drawBitmap(0, 0, g_frame, kawaii::kWidth, kawaii::kHeight, SSD1306_WHITE);
  display.display();
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

  const kawaii::Mood mood = currentMood(mode, animation, now, sleepPhase);
  kawaii::Expression target = kawaii::expressionFor(mood);
  if (mood == kawaii::Mood::Talking) {
    kawaii::applyVoice(target, currentVoice(now));
  }
  g_lastMood = mood;

  stepAnimator(target, now);
  drawFrame(now);
}

}  // namespace

extern const EyeStyleRenderer kKawaiiEyeStyle = {
  "kawaii",
  true,
  true,
  drawKawaii,
};
