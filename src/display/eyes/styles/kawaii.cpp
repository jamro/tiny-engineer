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
#include "display/eyes/styles/kawaii/director.h"
#include "display/eyes/styles/kawaii/speech.h"
#include "display/eyes/styles/kawaii/tracker.h"
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

// Work "going well": accents come more often the longer it has been since
// an error, abort or dead, reaching their most frequent after this long.
constexpr uint32_t kFlowRampMs = 90000;

uint8_t g_frame[kawaii::kFrameBytes];
kawaii::Animator g_animator;
kawaii::MoodDirector g_director(anim::randRangeMs);
bool g_directing = false;  // the director owns the mood (idle or a working mode)
bool g_wasTyping = false;
uint32_t g_calmSinceMs = 0;  // last error / abort / dead
kawaii::Babbler g_babbler(anim::randRangeMs);
kawaii::CodeReader g_reader(anim::randRangeMs);
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

// Modes with a personality (a base mood plus accents); the rest map straight
// to one mood.
bool activityFor(EyeMode mode, kawaii::Activity& out) {
  switch (mode) {
    case EyeMode::Idle:
      out = kawaii::Activity::Idle;
      return true;
    case EyeMode::Typing:
      out = kawaii::Activity::Typing;
      return true;
    case EyeMode::Reading:
      out = kawaii::Activity::Reading;
      return true;
    case EyeMode::Thinking:
      out = kawaii::Activity::Thinking;
      return true;
    default:
      return false;
  }
}

bool isSetback(EyeMode mode) {
  return mode == EyeMode::Error || mode == EyeMode::Abort || mode == EyeMode::Dead;
}

// 0 right after a setback .. 1 once work has gone well for kFlowRampMs.
float workFlow(uint32_t now) {
  return kawaii::clamp01((float)(now - g_calmSinceMs) / (float)kFlowRampMs);
}

kawaii::Mood currentMood(EyeMode mode, AnimationId animation, uint32_t now, EyeStyleSleepPhase sleepPhase) {
  const bool sleeping = sleepPhase == EyeStyleSleepPhase::Closing || animation == AnimationId::Sleep;
  kawaii::Activity activity = kawaii::Activity::Idle;
  if (sleeping || !activityFor(mode, activity)) {
    g_directing = false;
    return sleeping ? kawaii::Mood::Asleep : modeMood(mode);
  }

  if (!g_directing || g_director.activity() != activity) {
    g_director.reset(now, activity);  // start from the base mood
    g_directing = true;
  }
  return g_director.update(now, workFlow(now));
}

// Typing: the eyes read code whatever the face, accents included, and skip
// the choreography's typing jitter.
void applyTypingGaze(kawaii::Expression& target, uint32_t now) {
  if (!g_wasTyping) {
    g_reader.reset(now);  // typing just began: pick a spot to read
  }
  const kawaii::Gaze gaze = g_reader.update(now);
  target.gazeX = gaze.x;
  target.gazeY = gaze.y;
  target.followGlance = 0.0f;
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

// Center of an eye; integer halves, so a height change alone does not read
// as a glance.
float centerOf(int16_t pos, int16_t size) {
  return (float)(pos + size / 2);
}

// An eye where the choreography put it, with its glance scaled by `follow`
// (moods can calm a jittery glance down). Heights still follow fully.
kawaii::EyePose eyePose(const Eye& eye, const Eye& rest, float follow) {
  const float restX = rest.x + rest.width * 0.5f;
  const float restY = rest.y + rest.height * 0.5f;
  return {
    restX + follow * (eye.x + eye.width * 0.5f - restX),
    restY + follow * (eye.y + eye.height * 0.5f - restY),
    (float)eye.height / (float)rest.height
  };
}

void drawFrame(uint32_t now) {
  const Eye& left = leftEye();
  const float follow = g_animator.current().followGlance;
  const float glanceX = (centerOf(left.x, left.width) -
    centerOf(eyes::DEFAULT_LEFT.x, eyes::DEFAULT_LEFT.width)) / kGlanceRangeXPx;
  const float glanceY = (centerOf(left.y, left.height) -
    centerOf(eyes::DEFAULT_LEFT.y, eyes::DEFAULT_LEFT.height)) / kGlanceRangeYPx;

  // The pupils look where the shared choreography moved the eyes.
  const kawaii::Pose pose = {
    eyePose(left, eyes::DEFAULT_LEFT, follow),
    eyePose(rightEye(), eyes::DEFAULT_RIGHT, follow),
    blinkOpenAmount(),
    follow * glanceX,
    follow * glanceY,
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

  if (isSetback(mode)) {
    g_calmSinceMs = now;
  }

  const kawaii::Mood mood = currentMood(mode, animation, now, sleepPhase);
  kawaii::Expression target = kawaii::expressionFor(mood);
  if (mood == kawaii::Mood::Talking) {
    kawaii::applyVoice(target, currentVoice(now));
  }
  const bool typing = mode == EyeMode::Typing && mood != kawaii::Mood::Asleep;
  if (typing) {
    applyTypingGaze(target, now);
  }
  g_wasTyping = typing;
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
