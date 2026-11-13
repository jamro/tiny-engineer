#include "display/eyes/styles/eye_style.h"

#include <Adafruit_SSD1306.h>

#include "animation/dead.h"
#include "animation/error.h"
#include "animation/abort.h"
#include "animation/attention.h"
#include "animation/welcome.h"
#include "animation/util.h"
#include "audio/audio_motion.h"
#include "audio/wav_stream.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/styles/kawaii/animator.h"
#include "display/eyes/styles/kawaii/compose.h"
#include "display/eyes/styles/kawaii/face.h"
#include "display/eyes/styles/kawaii/lines.h"
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
struct SpokenClip;
const SpokenClip* g_mouthing = nullptr;  // the clip the mouth is saying
uint32_t g_calmSinceMs = 0;  // last error / abort / dead
kawaii::BellCelebration g_bell;
uint32_t g_ringStartedMs = 0;
bool g_ringing = false;
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
      return kawaii::Mood::Curious;  // ringMood() runs the celebration
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

// The bell: suspense through the wind-up, joy on the ding, a fading shiver.
kawaii::Expression ringFace(uint32_t modeStartedMs, uint32_t now) {
  if (!g_ringing || modeStartedMs != g_ringStartedMs) {
    g_bell.reset(now);
    g_ringStartedMs = modeStartedMs;
  }
  return g_bell.update(now, wavStreamIsPlaying(WavClip::Bell));
}

kawaii::Mood currentMood(
  EyeMode mode, AnimationId animation, uint32_t modeStartedMs, uint32_t now,
  EyeStyleSleepPhase sleepPhase
) {
  const bool sleeping = sleepPhase == EyeStyleSleepPhase::Closing || animation == AnimationId::Sleep;
  if (!sleeping && mode == EyeMode::Ring) {
    g_directing = false;
    return kawaii::Mood::Bliss;  // drawKawaii() asks ringFace() for the face
  }
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

// A voice clip the mouth can say. With the stock clip it mouths the actual
// words; a custom clip (its own .cue timings) says something else, so the
// mouth babbles through its spoken phrases instead.
struct SpokenClip {
  kawaii::Line line;
  bool (*started)();
  uint32_t (*elapsed)(uint32_t now);
  const uint32_t* endMs;
  size_t (*cues)(uint32_t* out);          // the live .cue values, field order
  bool (*inPhrase)(uint32_t elapsedMs);  // babble window for custom clips
};

constexpr size_t kMaxCues = 4;

size_t welcomeCues(uint32_t* out) {
  out[0] = WELCOME_AUDIO_GREETING_END_MS;
  out[1] = WELCOME_AUDIO_PAUSE_END_MS;
  out[2] = WELCOME_AUDIO_QUESTION_END_MS;
  out[3] = WELCOME_AUDIO_END_MS;
  return 4;
}

bool welcomeInPhrase(uint32_t t) {
  return t < WELCOME_AUDIO_GREETING_END_MS ||
    (t >= WELCOME_AUDIO_PAUSE_END_MS && t < WELCOME_AUDIO_QUESTION_END_MS);
}

size_t attentionCues(uint32_t* out) {
  out[0] = ATTENTION_AUDIO_PST_END_MS;
  out[1] = ATTENTION_AUDIO_HUMAN_END_MS;
  out[2] = ATTENTION_AUDIO_END_MS;
  return 3;
}

size_t errorCues(uint32_t* out) {
  out[0] = ERROR_AUDIO_UHOH_END_MS;
  out[1] = ERROR_AUDIO_HUMAN_END_MS;
  out[2] = ERROR_AUDIO_PROBLEM_END_MS;
  out[3] = ERROR_AUDIO_END_MS;
  return 4;
}

size_t abortCues(uint32_t* out) {
  out[0] = ABORT_AUDIO_FINE_END_MS;
  out[1] = ABORT_AUDIO_DIDNT_WANT_END_MS;
  out[2] = ABORT_AUDIO_FINISH_END_MS;
  out[3] = ABORT_AUDIO_END_MS;
  return 4;
}

size_t deadCues(uint32_t* out) {
  out[0] = DEAD_AUDIO_SHUTDOWN_MS;
  out[1] = DEAD_AUDIO_END_MS;
  return 2;
}

bool alwaysInPhrase(uint32_t /*t*/) {
  return true;  // short phrases; the babbler pauses on its own
}

const SpokenClip kWelcomeClip = {
  kawaii::Line::HelloHuman, welcomeAudioStarted, welcomeAudioElapsed,
  &WELCOME_AUDIO_END_MS, welcomeCues, welcomeInPhrase,
};
const SpokenClip kAttentionClip = {
  kawaii::Line::PsstHuman, attentionAudioStarted, attentionAudioElapsed,
  &ATTENTION_AUDIO_END_MS, attentionCues, alwaysInPhrase,
};
const SpokenClip kErrorClip = {
  kawaii::Line::UhOhHuman, errorAudioStarted, errorAudioElapsed,
  &ERROR_AUDIO_END_MS, errorCues, alwaysInPhrase,
};
const SpokenClip kAbortClip = {
  kawaii::Line::FineAnyway, abortAudioStarted, abortAudioElapsed,
  &ABORT_AUDIO_END_MS, abortCues, alwaysInPhrase,
};
// dead.wav plays through the error warning, so it shares error's timer.
const SpokenClip kDeadClip = {
  kawaii::Line::ShuttingDown, errorAudioStarted, errorAudioElapsed,
  &DEAD_AUDIO_END_MS, deadCues, alwaysInPhrase,
};

const SpokenClip* spokenClipFor(EyeMode mode) {
  switch (mode) {
    case EyeMode::Welcome:
      return &kWelcomeClip;
    case EyeMode::Attention:
      return &kAttentionClip;
    case EyeMode::Error:
      return &kErrorClip;
    case EyeMode::Abort:
      return &kAbortClip;
    case EyeMode::Dead:
      return &kDeadClip;
    default:
      return nullptr;
  }
}

bool isStockClip(const SpokenClip& clip) {
  uint32_t cues[kMaxCues];
  const size_t count = clip.cues(cues);
  return kawaii::fitsStock(clip.line, cues, count);
}

// Mouths the clip while it plays; false before it starts and after it ends.
bool applySpokenClip(kawaii::Expression& target, const SpokenClip& clip, uint32_t now) {
  if (!clip.started()) {
    return false;
  }
  const uint32_t t = clip.elapsed(now);
  if (t >= *clip.endMs) {
    return false;
  }
  if (g_mouthing != &clip) {
    g_babbler.reset(now);
  }

  kawaii::Voice voice = {0.0f, kawaii::Vowel::Ah};
  if (isStockClip(clip)) {
    voice = kawaii::say(clip.line, t);
  } else if (clip.inPhrase(t)) {
    voice = g_babbler.babble(now);
  }
  kawaii::speakingMouth(target);
  kawaii::applyVoice(target, voice);
  return true;
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
// (moods can calm a jittery glance down). Heights follow unless `ownHeight`.
kawaii::EyePose eyePose(const Eye& eye, const Eye& rest, float follow, bool ownHeight) {
  const float restX = rest.x + rest.width * 0.5f;
  const float restY = rest.y + rest.height * 0.5f;
  return {
    restX + follow * (eye.x + eye.width * 0.5f - restX),
    restY + follow * (eye.y + eye.height * 0.5f - restY),
    ownHeight ? 1.0f : (float)eye.height / (float)rest.height
  };
}

// The ring's choreography squashes the eyes into small circles; the bell
// celebration shapes the eyes itself.
bool ownsEyeHeight(EyeMode mode) {
  return mode == EyeMode::Ring;
}

void drawFrame(EyeMode mode, uint32_t now) {
  const Eye& left = leftEye();
  const float follow = g_animator.current().followGlance;
  const float glanceX = (centerOf(left.x, left.width) -
    centerOf(eyes::DEFAULT_LEFT.x, eyes::DEFAULT_LEFT.width)) / kGlanceRangeXPx;
  const float glanceY = (centerOf(left.y, left.height) -
    centerOf(eyes::DEFAULT_LEFT.y, eyes::DEFAULT_LEFT.height)) / kGlanceRangeYPx;

  // The pupils look where the shared choreography moved the eyes.
  const kawaii::Pose pose = {
    eyePose(left, eyes::DEFAULT_LEFT, follow, ownsEyeHeight(mode)),
    eyePose(rightEye(), eyes::DEFAULT_RIGHT, follow, ownsEyeHeight(mode)),
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
  uint32_t modeStartedMs,
  uint32_t now,
  EyeStyleSleepPhase sleepPhase
) {
  if (!oledAvailable) {
    return;
  }

  if (isSetback(mode)) {
    g_calmSinceMs = now;
  }

  const kawaii::Mood mood = currentMood(mode, animation, modeStartedMs, now, sleepPhase);
  const bool ringing = mode == EyeMode::Ring && mood != kawaii::Mood::Asleep;
  kawaii::Expression target =
    ringing ? ringFace(modeStartedMs, now) : kawaii::expressionFor(mood);
  g_ringing = ringing;
  if (mood == kawaii::Mood::Talking) {
    kawaii::applyVoice(target, currentVoice(now));
  }
  const SpokenClip* clip = mood == kawaii::Mood::Asleep ? nullptr : spokenClipFor(mode);
  g_mouthing = clip != nullptr && applySpokenClip(target, *clip, now) ? clip : nullptr;
  const bool typing = mode == EyeMode::Typing && mood != kawaii::Mood::Asleep;
  if (typing) {
    applyTypingGaze(target, now);
  }
  g_wasTyping = typing;
  g_lastMood = mood;

  stepAnimator(target, now);
  drawFrame(mode, now);
}

}  // namespace

extern const EyeStyleRenderer kKawaiiEyeStyle = {
  "kawaii",
  true,
  true,
  drawKawaii,
};
