#include <cmath>
#include <cstring>
#include <unity.h>

#include "display/eyes/styles/kawaii/animator.h"
#include "display/eyes/styles/kawaii/burst.h"
#include "display/eyes/styles/kawaii/compose.h"
#include "display/eyes/styles/kawaii/expression.h"
#include "display/eyes/styles/kawaii/face.h"
#include "display/eyes/styles/kawaii/lines.h"
#include "display/eyes/styles/kawaii/director.h"
#include "display/eyes/styles/kawaii/speech.h"
#include "display/eyes/styles/kawaii/tracker.h"

using namespace kawaii;

namespace {

uint8_t g_frame[kFrameBytes];
uint32_t g_seed = 1;

uint32_t testRand(uint32_t lo, uint32_t hi) {
  g_seed = g_seed * 1664525u + 1013904223u;
  return hi <= lo ? lo : lo + (g_seed >> 8) % (hi - lo);
}

EyeShape roundEye(float cx, int8_t side) {
  EyeShape e = {};
  e.kind = EyeKind::Round;
  e.side = side;
  e.cx = cx;
  e.cy = 16.0f;
  e.rx = kEyeRadius;
  e.ry = kEyeRadius;
  e.open = 1.0f;
  e.pupil = kPupilRadius * kEyeRadius;
  e.sparkle = 1.0f;
  return e;
}

Face plainFace() {
  Face f = {};
  f.left = roundEye(32.0f, 1);
  f.right = roundEye(96.0f, -1);
  f.mouth.cx = 64.0f;
  f.mouth.cy = 23.5f;
  f.mouth.width = 0.0f;
  return f;
}

int litCount(int x0, int y0, int x1, int y1) {
  int n = 0;
  for (int y = y0; y <= y1; y++) {
    for (int x = x0; x <= x1; x++) {
      n += pixelAt(g_frame, x, y) ? 1 : 0;
    }
  }
  return n;
}

}  // namespace

void setUp() {
  memset(g_frame, 0, sizeof(g_frame));
  g_seed = 1;
}

void tearDown() {}

void test_render_rejects_small_buffer() {
  const Face f = plainFace();
  TEST_ASSERT_FALSE(render(f, g_frame, kFrameBytes - 1));
  TEST_ASSERT_FALSE(render(f, nullptr, kFrameBytes));
  TEST_ASSERT_TRUE(render(f, g_frame, kFrameBytes));
}

void test_render_is_pure() {
  Face f = plainFace();
  f.blush = 1.0f;
  f.mouth.width = 9.0f;
  f.mouth.openDown = 3.0f;

  static uint8_t again[kFrameBytes];
  memset(again, 0xFF, sizeof(again));  // stale pixels must not survive
  render(f, g_frame, kFrameBytes);
  render(plainFace(), again, kFrameBytes);  // unrelated frame in between
  render(f, again, kFrameBytes);
  TEST_ASSERT_EQUAL_MEMORY(g_frame, again, kFrameBytes);
}

void test_open_eye_has_a_pupil_and_catch_light() {
  render(plainFace(), g_frame, kFrameBytes);

  TEST_ASSERT_TRUE(pixelAt(g_frame, 24, 16));   // white of the eye
  TEST_ASSERT_FALSE(pixelAt(g_frame, 33, 17));  // pupil, at the center
  TEST_ASSERT_TRUE(pixelAt(g_frame, 30, 14));   // catch-light, upper left in the pupil
  TEST_ASSERT_FALSE(pixelAt(g_frame, 32, 2));   // above the eye
  TEST_ASSERT_FALSE(pixelAt(g_frame, 64, 16));  // between the eyes
  TEST_ASSERT_FALSE(pixelAt(g_frame, 97, 17));
}

// Mean x and y of the dark pixels inside the left eye (the pupil).
void pupilCentroid(float& x, float& y) {
  float sx = 0.0f;
  float sy = 0.0f;
  int n = 0;
  for (int py = 6; py <= 26; py++) {
    for (int px = 22; px <= 42; px++) {
      const float dx = px + 0.5f - 32.0f;
      const float dy = py + 0.5f - 16.0f;
      if (dx * dx + dy * dy < 10.0f * 10.0f && !pixelAt(g_frame, px, py)) {  // inside the eye
        sx += px;
        sy += py;
        n++;
      }
    }
  }
  x = n > 0 ? sx / n : 0.0f;
  y = n > 0 ? sy / n : 0.0f;
}

void test_pupil_follows_the_gaze() {
  float cx = 0.0f;
  float cy = 0.0f;
  render(plainFace(), g_frame, kFrameBytes);
  pupilCentroid(cx, cy);

  Face f = plainFace();
  f.left.gazeX = 1.0f;
  render(f, g_frame, kFrameBytes);
  float x = 0.0f;
  float y = 0.0f;
  pupilCentroid(x, y);
  TEST_ASSERT_TRUE(x > cx + 1.0f);

  f = plainFace();
  f.left.gazeY = -1.0f;
  render(f, g_frame, kFrameBytes);
  pupilCentroid(x, y);
  TEST_ASSERT_TRUE(y < cy - 1.0f);
}

// Lit pixels of the left eye's box.
int leftEyeLit(const uint8_t* frame) {
  int n = 0;
  for (int y = 0; y < kHeight; y++) {
    for (int x = 0; x < 64; x++) {
      n += pixelAt(frame, x, y) ? 1 : 0;
    }
  }
  return n;
}

// Gliding the gaze must not change how big the pupil looks.
void test_pupil_keeps_its_size_while_it_moves() {
  static uint8_t bare[kFrameBytes];
  int size = -1;
  for (int i = -20; i <= 20; i++) {
    Face f = plainFace();
    f.right.rx = 0.0f;
    f.left.sparkle = 0.0f;  // count the pupil alone
    f.left.gazeX = i * 0.02f;  // within the travel that never tucks under the rim
    f.left.gazeY = -i * 0.015f;
    render(f, g_frame, kFrameBytes);
    f.left.pupil = 0.0f;
    render(f, bare, kFrameBytes);

    const int dark = leftEyeLit(bare) - leftEyeLit(g_frame);
    TEST_ASSERT_TRUE(dark > 100);
    if (size < 0) {
      size = dark;
    }
    TEST_ASSERT_EQUAL_INT(size, dark);
  }
}

// The pupil never reaches the eye's outline: every row and column keeps the
// same lit extremes as the same eye drawn with no pupil.
void test_pupil_keeps_a_rim() {
  static uint8_t bare[kFrameBytes];
  const float lids[][2] = {{0.0f, 0.0f}, {0.32f, 0.55f}, {0.45f, 0.0f}, {0.15f, -0.5f}};
  const float happies[] = {0.0f, 0.22f, 0.5f};

  for (const auto& lid : lids) {
    for (float happy : happies) {
      for (int dir = 0; dir < 8; dir++) {
        Face f = plainFace();
        f.right.rx = 0.0f;  // left eye only
        f.left.lid = lid[0];
        f.left.tilt = lid[1];
        f.left.happy = happy;
        f.left.gazeX = cosf(dir * 0.785398f) * 1.5f;  // past the edge on purpose
        f.left.gazeY = sinf(dir * 0.785398f) * 1.5f;
        render(f, g_frame, kFrameBytes);
        f.left.pupil = 0.0f;
        render(f, bare, kFrameBytes);

        for (int y = 0; y < kHeight; y++) {
          int first = -1;
          int last = -1;
          int bareFirst = -1;
          int bareLast = -1;
          for (int x = 0; x < 64; x++) {
            if (pixelAt(g_frame, x, y)) {
              first = first < 0 ? x : first;
              last = x;
            }
            if (pixelAt(bare, x, y)) {
              bareFirst = bareFirst < 0 ? x : bareFirst;
              bareLast = x;
            }
          }
          TEST_ASSERT_EQUAL_INT(bareFirst, first);
          TEST_ASSERT_EQUAL_INT(bareLast, last);
        }
        for (int x = 0; x < 64; x++) {
          int top = -1;
          int bareTop = -1;
          int bottom = -1;
          int bareBottom = -1;
          for (int y = 0; y < kHeight; y++) {
            if (pixelAt(g_frame, x, y)) {
              top = top < 0 ? y : top;
              bottom = y;
            }
            if (pixelAt(bare, x, y)) {
              bareTop = bareTop < 0 ? y : bareTop;
              bareBottom = y;
            }
          }
          TEST_ASSERT_EQUAL_INT(bareTop, top);
          TEST_ASSERT_EQUAL_INT(bareBottom, bottom);
        }
      }
    }
  }
}

void test_typing_reads_lines_then_darts() {
  CodeReader reader(testRand);
  reader.reset(0);

  Gaze prev = reader.update(0);
  int words = 0;
  int newLines = 0;
  int darts = 0;
  for (uint32_t now = 33; now < 30000u; now += 33) {
    const Gaze g = reader.update(now);
    TEST_ASSERT_TRUE(g.x >= -0.6f && g.x <= 0.8f);
    TEST_ASSERT_TRUE(g.y >= -0.4f && g.y <= 0.81f);
    if (g.x != prev.x || g.y != prev.y) {
      if (g.y == prev.y && g.x > prev.x) {
        words++;
        TEST_ASSERT_TRUE(g.x - prev.x < 0.15f);  // a word at a time
      } else if (g.y > prev.y && g.y - prev.y < 0.13f && g.x < prev.x) {
        newLines++;
      } else {
        darts++;
      }
    }
    prev = g;
  }
  TEST_ASSERT_TRUE(words > 30);
  TEST_ASSERT_TRUE(newLines > 3);
  TEST_ASSERT_TRUE(darts > 3);
}

void test_full_smile_has_no_pupil() {
  static uint8_t bare[kFrameBytes];
  Face f = plainFace();
  f.left.happy = 1.0f;
  render(f, g_frame, kFrameBytes);
  f.left.pupil = 0.0f;
  render(f, bare, kFrameBytes);
  TEST_ASSERT_EQUAL_MEMORY(bare, g_frame, kFrameBytes);
}

void test_closed_eye_is_a_lid_line() {
  Face f = plainFace();
  f.left.open = 0.0f;
  render(f, g_frame, kFrameBytes);

  TEST_ASSERT_EQUAL_INT(0, litCount(21, 5, 43, 13));
  TEST_ASSERT_TRUE(litCount(21, 14, 43, 19) >= 30);
}

void test_happy_eye_is_a_crescent() {
  Face f = plainFace();
  f.left.happy = 1.0f;
  render(f, g_frame, kFrameBytes);

  TEST_ASSERT_TRUE(pixelAt(g_frame, 32, 6));    // top arc
  TEST_ASSERT_FALSE(pixelAt(g_frame, 32, 16));  // carved center
  TEST_ASSERT_FALSE(pixelAt(g_frame, 32, 24));  // carved bottom
}

void test_lid_and_tilt_cut_the_top() {
  Face f = plainFace();
  f.left.lid = 0.3f;
  f.left.tilt = 0.6f;  // angry: inner (right) side of the left eye lower
  render(f, g_frame, kFrameBytes);

  TEST_ASSERT_TRUE(litCount(23, 6, 27, 10) > 0);
  TEST_ASSERT_EQUAL_INT(0, litCount(37, 5, 42, 11));
}

void test_blush_keeps_a_gap_from_the_eyes() {
  Face f = plainFace();
  f.blush = 1.0f;
  render(f, g_frame, kFrameBytes);

  Face noBlush = plainFace();
  static uint8_t eyes[kFrameBytes];
  render(noBlush, eyes, kFrameBytes);

  int blushPixels = 0;
  for (int y = 0; y < kHeight; y++) {
    for (int x = 0; x < kWidth; x++) {
      if (!pixelAt(g_frame, x, y) || pixelAt(eyes, x, y)) {
        continue;
      }
      blushPixels++;
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          TEST_ASSERT_FALSE(pixelAt(eyes, x + dx, y + dy));
        }
      }
    }
  }
  TEST_ASSERT_TRUE(blushPixels >= 10);
}

void test_hollow_mouth_is_an_outline() {
  Face f = plainFace();
  f.mouth.width = 7.0f;
  f.mouth.openUp = 3.0f;
  f.mouth.openDown = 3.0f;
  f.mouth.hollow = true;
  render(f, g_frame, kFrameBytes);
  TEST_ASSERT_FALSE(pixelAt(g_frame, 64, 23));
  TEST_ASSERT_TRUE(litCount(61, 20, 67, 27) >= 10);

  f.mouth.hollow = false;
  render(f, g_frame, kFrameBytes);
  TEST_ASSERT_TRUE(pixelAt(g_frame, 64, 23));
}

void test_every_kind_draws_and_clips() {
  const EyeKind kinds[] = {
    EyeKind::Round, EyeKind::Chevron, EyeKind::Heart, EyeKind::Cross
  };

  for (EyeKind kind : kinds) {
    Face f = plainFace();
    f.left.kind = kind;
    f.right.kind = kind;
    f.right.cx = 126.0f;  // half off the panel
    f.left.cy = -4.0f;
    f.blush = 1.0f;
    f.glint = 3.0f;
    f.mouth.width = 60.0f;  // wider than the column buffer: skipped, no overflow
    TEST_ASSERT_TRUE(render(f, g_frame, kFrameBytes));
    TEST_ASSERT_TRUE(litCount(100, 0, 127, 31) > 0);
  }
}

void test_mood_names_are_unique() {
  for (size_t i = 0; i < (size_t)Mood::Count; i++) {
    const char* a = moodName((Mood)i);
    TEST_ASSERT_NOT_NULL(a);
    for (size_t j = i + 1; j < (size_t)Mood::Count; j++) {
      TEST_ASSERT_TRUE(strcmp(a, moodName((Mood)j)) != 0);
    }
  }
  TEST_ASSERT_NULL(moodName(Mood::Count));
}

void test_shape_moods_use_their_kind() {
  TEST_ASSERT_TRUE(expressionFor(Mood::Panic).kind == EyeKind::Chevron);
  TEST_ASSERT_TRUE(expressionFor(Mood::Dizzy).kind == EyeKind::Cross);
  TEST_ASSERT_TRUE(expressionFor(Mood::Love).kind == EyeKind::Heart);
  TEST_ASSERT_TRUE(expressionFor(Mood::Smile).kind == EyeKind::Round);
}

void test_kind_swap_blinks_through() {
  Animator a;
  a.snap(expressionFor(Mood::Content));
  const Expression panic = expressionFor(Mood::Panic);

  a.update(panic, 33);
  TEST_ASSERT_TRUE(a.kindOpen() < 1.0f);
  TEST_ASSERT_TRUE(a.current().kind == EyeKind::Round);

  bool sawShut = false;
  for (int i = 0; i < 20; i++) {
    a.update(panic, 33);
    sawShut = sawShut || a.kindOpen() == 0.0f;
  }
  TEST_ASSERT_TRUE(sawShut);
  TEST_ASSERT_TRUE(a.current().kind == EyeKind::Chevron);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, a.kindOpen());
}

void test_expression_eases_and_settles() {
  Animator a;
  a.snap(expressionFor(Mood::Content));
  const Expression grin = expressionFor(Mood::Grin);

  a.update(grin, 33);
  TEST_ASSERT_TRUE(a.current().happyLeft > 0.0f);
  TEST_ASSERT_TRUE(a.current().happyLeft < 1.0f);

  float peak = 0.0f;
  for (int i = 0; i < 60; i++) {
    a.update(grin, 33);
    peak = a.current().happyLeft > peak ? a.current().happyLeft : peak;
  }
  TEST_ASSERT_TRUE(peak < 1.25f);  // a little pop, not a wobble
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 1.0f, a.current().happyLeft);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, grin.mouthOpenDown, a.current().mouthOpenDown);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, grin.blush, a.current().blush);
}

void test_long_gap_is_clamped() {
  Animator a;
  a.snap(expressionFor(Mood::Content));
  a.update(expressionFor(Mood::Excited), 60000);
  TEST_ASSERT_TRUE(a.current().scale > 0.5f);
  TEST_ASSERT_TRUE(a.current().scale < 1.5f);
}

void test_voice_opens_the_resting_mouth() {
  const Expression rest = expressionFor(Mood::Talking);

  Expression closed = rest;
  applyVoice(closed, {0.0f, Vowel::Ah});
  TEST_ASSERT_EQUAL_FLOAT(rest.mouthWidth, closed.mouthWidth);
  TEST_ASSERT_EQUAL_FLOAT(rest.mouthOpenDown, closed.mouthOpenDown);

  Expression ah = rest;
  applyVoice(ah, {1.0f, Vowel::Ah});
  TEST_ASSERT_TRUE(ah.mouthOpenDown > rest.mouthOpenDown + 3.0f);

  Expression ee = rest;
  applyVoice(ee, {1.0f, Vowel::Ee});
  TEST_ASSERT_TRUE(ee.mouthWidth > ah.mouthWidth);
  TEST_ASSERT_TRUE(ee.mouthOpenDown < ah.mouthOpenDown);

  Expression oo = rest;
  applyVoice(oo, {1.0f, Vowel::Oo});
  TEST_ASSERT_TRUE(oo.mouthWidth < rest.mouthWidth);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, oo.mouthHollow);

  Expression murmur = rest;
  applyVoice(murmur, {0.2f, Vowel::Oo});  // too small to read as a ring
  TEST_ASSERT_EQUAL_FLOAT(0.0f, murmur.mouthHollow);

  Expression loud = rest;
  applyVoice(loud, {5.0f, Vowel::Ah});  // clamped to fully open
  TEST_ASSERT_EQUAL_FLOAT(ah.mouthOpenDown, loud.mouthOpenDown);
}

void test_babble_speaks_in_phrases() {
  Babbler babbler(testRand);
  babbler.reset(0);

  uint32_t openMs = 0;
  uint32_t quietRunMs = 0;
  uint32_t longestQuietMs = 0;
  bool sawVowel[(size_t)Vowel::Count] = {};

  for (uint32_t now = 0; now < 20000u; now += 33) {
    const Voice v = babbler.babble(now);
    TEST_ASSERT_TRUE(v.level >= 0.0f && v.level <= 1.0f);
    sawVowel[(size_t)v.vowel] = true;
    openMs += v.level > 0.3f ? 33 : 0;
    quietRunMs = v.level < 0.01f ? quietRunMs + 33 : 0;
    longestQuietMs = quietRunMs > longestQuietMs ? quietRunMs : longestQuietMs;
  }

  TEST_ASSERT_TRUE(openMs > 20000u / 5);   // the mouth really moves
  TEST_ASSERT_TRUE(longestQuietMs >= 200);  // with pauses between phrases
  TEST_ASSERT_TRUE(sawVowel[(size_t)Vowel::Ah]);
  TEST_ASSERT_TRUE(sawVowel[(size_t)Vowel::Ee]);
  TEST_ASSERT_TRUE(sawVowel[(size_t)Vowel::Oo]);
}

void test_babble_survives_millis_wrap() {
  Babbler babbler(testRand);
  const uint32_t start = 0xFFFFFFFFu - 2000u;
  babbler.reset(start);

  float loudest = 0.0f;
  for (uint32_t t = 0; t < 6000u; t += 33) {
    const float level = babbler.babble(start + t).level;
    TEST_ASSERT_TRUE(level >= 0.0f && level <= 1.0f);
    loudest = level > loudest ? level : loudest;
  }
  TEST_ASSERT_TRUE(loudest > 0.3f);
}

void test_follow_tracks_audio() {
  Babbler babbler(testRand);
  babbler.reset(0);

  TEST_ASSERT_EQUAL_FLOAT(0.8f, babbler.follow(0, 0.8f, 0.0f, true).level);
  TEST_ASSERT_TRUE(babbler.follow(33, 0.8f, 0.0f, false).level < 0.4f);  // gate closed

  bool sawVowel[(size_t)Vowel::Count] = {};
  for (uint32_t now = 0; now < 5000u; now += 33) {
    const bool onset = (now / 33) % 6 == 0;
    sawVowel[(size_t)babbler.follow(now, 0.6f, onset ? 0.6f : 0.0f, true).vowel] = true;
  }
  TEST_ASSERT_TRUE(sawVowel[(size_t)Vowel::Ah] + sawVowel[(size_t)Vowel::Ee] + sawVowel[(size_t)Vowel::Oo] >= 2);
}

void test_idle_is_mostly_smiles_with_accents() {
  MoodDirector idle(testRand);
  idle.reset(0, Activity::Idle);

  uint32_t smileMs = 0;
  uint32_t totalMs = 0;
  bool sawAccent[(size_t)Mood::Count] = {};

  for (uint32_t now = 0; now < 10u * 60u * 1000u; now += 33) {
    const Mood m = idle.update(now);
    TEST_ASSERT_TRUE(isIdleMood(m));
    sawAccent[(size_t)m] = true;
    if (m == Mood::Content || m == Mood::Smile) {
      smileMs += 33;
    }
    totalMs += 33;
  }

  TEST_ASSERT_TRUE(smileMs * 100 / totalMs >= 65);
  TEST_ASSERT_TRUE(sawAccent[(size_t)Mood::Grin]);
  TEST_ASSERT_TRUE(sawAccent[(size_t)Mood::WinkLeft] || sawAccent[(size_t)Mood::WinkRight]);
  TEST_ASSERT_TRUE(sawAccent[(size_t)Mood::Excited]);
}

void test_idle_survives_millis_wrap() {
  MoodDirector idle(testRand);
  const uint32_t start = 0xFFFFFFFFu - 20000u;
  idle.reset(start, Activity::Idle);

  bool sawAccent = false;
  for (uint32_t t = 0; t < 60000u; t += 33) {
    const Mood m = idle.update(start + t);
    sawAccent = sawAccent || (m != Mood::Content && m != Mood::Smile);
  }
  TEST_ASSERT_TRUE(sawAccent);
}

// Ten minutes of an activity: share of time on the base mood, which moods
// appeared, and how many accents started.
struct ActivityRun {
  uint32_t baseMs = 0;
  uint32_t accents = 0;
  bool saw[(size_t)Mood::Count] = {};
};

ActivityRun runActivity(Activity activity, Mood base, float flow) {
  MoodDirector director(testRand);
  director.reset(0, activity);
  ActivityRun run;
  Mood prev = base;
  for (uint32_t now = 0; now < 10u * 60u * 1000u; now += 33) {
    const Mood m = director.update(now, flow);
    run.saw[(size_t)m] = true;
    run.baseMs += m == base ? 33 : 0;
    run.accents += (m != base && prev == base) ? 1 : 0;
    prev = m;
  }
  return run;
}

void test_working_moods_are_mostly_focused_with_positive_accents() {
  struct Case {
    Activity activity;
    Mood base;
    Mood allowed[4];
  };
  const Case cases[] = {
    {Activity::Typing, Mood::Focused, {Mood::Pleased, Mood::Grin, Mood::WinkLeft, Mood::WinkRight}},
    {Activity::Reading, Mood::Reading, {Mood::Pleased, Mood::Curious, Mood::Grin, Mood::Grin}},
    {Activity::Thinking, Mood::Thinking, {Mood::Aha, Mood::Pleased, Mood::Aha, Mood::Aha}},
  };

  for (const Case& c : cases) {
    const ActivityRun run = runActivity(c.activity, c.base, 0.5f);
    TEST_ASSERT_TRUE(run.baseMs * 100 / (10u * 60u * 1000u) >= 75);  // mostly working
    TEST_ASSERT_TRUE(run.accents >= 10);                             // but not all the time
    TEST_ASSERT_TRUE(run.saw[(size_t)c.allowed[0]]);
    for (size_t m = 0; m < (size_t)Mood::Count; m++) {
      if (!run.saw[m] || (Mood)m == c.base) {
        continue;
      }
      bool ok = false;
      for (Mood a : c.allowed) {
        ok = ok || (Mood)m == a;
      }
      TEST_ASSERT_TRUE_MESSAGE(ok, moodName((Mood)m));
    }
  }
}

void test_work_going_well_brings_more_smiles() {
  const ActivityRun rough = runActivity(Activity::Typing, Mood::Focused, 0.0f);
  const ActivityRun smooth = runActivity(Activity::Typing, Mood::Focused, 1.0f);
  TEST_ASSERT_TRUE(smooth.accents > rough.accents * 3 / 2);
}

void test_mouth_line_ramps_between_keyframes() {
  const Viseme track[] = {{100, Vowel::Ah, 0}, {200, Vowel::Oo, 80}, {300, Vowel::Oo, 0}};
  TEST_ASSERT_EQUAL_FLOAT(0.0f, mouthLine(track, 3, 50).level);   // before
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.4f, mouthLine(track, 3, 150).level);
  TEST_ASSERT_TRUE(mouthLine(track, 3, 150).vowel == Vowel::Oo);  // opening toward "o"
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, mouthLine(track, 3, 200).level);
  TEST_ASSERT_TRUE(mouthLine(track, 3, 250).vowel == Vowel::Oo);  // closing keeps it
  TEST_ASSERT_EQUAL_FLOAT(0.0f, mouthLine(track, 3, 300).level);  // after
  TEST_ASSERT_EQUAL_FLOAT(0.0f, mouthLine(track, 0, 150).level);
}

void test_hello_human_says_the_words() {
  // Times are where the stock welcome.wav says each sound.
  TEST_ASSERT_TRUE(say(Line::HelloHuman, 210).level > 0.6f);   // "lo"
  TEST_ASSERT_TRUE(say(Line::HelloHuman, 210).vowel == Vowel::Oo);
  TEST_ASSERT_TRUE(say(Line::HelloHuman, 670).level > 0.6f);   // "man"
  TEST_ASSERT_TRUE(say(Line::HelloHuman, 670).vowel == Vowel::Ah);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, say(Line::HelloHuman, 1000).level);  // the pause
  TEST_ASSERT_TRUE(say(Line::HelloHuman, 1760).level < 0.1f);  // lips shut just before the "b"
  TEST_ASSERT_TRUE(say(Line::HelloHuman, 1860).vowel == Vowel::Ee);  // "buil"
  TEST_ASSERT_TRUE(say(Line::HelloHuman, 2340).vowel == Vowel::Ah);  // "day"
  TEST_ASSERT_EQUAL_FLOAT(0.0f, say(Line::HelloHuman, 2700).level);  // done

  const uint32_t stock[] = {800, 1340, 2400, 2680};
  const uint32_t custom[] = {900, 1340, 2400, 2680};
  TEST_ASSERT_TRUE(fitsStock(Line::HelloHuman, stock, 4));
  TEST_ASSERT_FALSE(fitsStock(Line::HelloHuman, custom, 4));  // a custom clip
  TEST_ASSERT_FALSE(fitsStock(Line::HelloHuman, stock, 3));   // wrong cue file
}

void test_psst_human_says_the_words() {
  // Times are where the stock attention.wav says each sound (the track
  // leads by 40 ms).
  TEST_ASSERT_TRUE(say(Line::PsstHuman, 80).level < 0.05f);    // lips shut for the "P"
  TEST_ASSERT_TRUE(say(Line::PsstHuman, 300).level > 0.2f);    // "sss": a small toothy hiss
  TEST_ASSERT_TRUE(say(Line::PsstHuman, 300).vowel == Vowel::Ee);
  TEST_ASSERT_TRUE(say(Line::PsstHuman, 870).level > 0.6f);    // "man"
  TEST_ASSERT_EQUAL_FLOAT(0.0f, say(Line::PsstHuman, 1200).level);  // the pause
  TEST_ASSERT_TRUE(say(Line::PsstHuman, 1760).level > 0.7f);   // "might"
  TEST_ASSERT_TRUE(say(Line::PsstHuman, 2620).vowel == Vowel::Oo);  // "look"
  TEST_ASSERT_EQUAL_FLOAT(0.0f, say(Line::PsstHuman, 2900).level);  // done

  const uint32_t stock[] = {640, 1540, 2960};
  const uint32_t custom[] = {640, 1600, 2960};
  TEST_ASSERT_TRUE(fitsStock(Line::PsstHuman, stock, 3));
  TEST_ASSERT_FALSE(fitsStock(Line::PsstHuman, custom, 3));  // a custom clip
}

void test_error_abort_dead_say_their_lines() {
  // "Uh-oh. Human, we have a problem."
  TEST_ASSERT_TRUE(say(Line::UhOhHuman, 220).vowel == Vowel::Oo);     // "oh"
  TEST_ASSERT_TRUE(say(Line::UhOhHuman, 860).level < 0.1f);          // lips shut for the "m"
  TEST_ASSERT_TRUE(say(Line::UhOhHuman, 1760).level > 0.7f);         // "pro"
  TEST_ASSERT_TRUE(say(Line::UhOhHuman, 1860).level < 0.1f);         // the "b"
  // "Fine. I didn't want to finish it anyway."
  TEST_ASSERT_TRUE(say(Line::FineAnyway, 100).level < 0.15f);        // "F": lip on teeth
  TEST_ASSERT_TRUE(say(Line::FineAnyway, 220).level > 0.6f);         // "fine"
  TEST_ASSERT_TRUE(say(Line::FineAnyway, 1320).vowel == Vowel::Ah);  // "want"
  // "Insufficient resources... shutting doooowwwnnn"
  TEST_ASSERT_TRUE(say(Line::ShuttingDown, 1040).vowel == Vowel::Oo);  // "sour"
  TEST_ASSERT_EQUAL_FLOAT(0.0f, say(Line::ShuttingDown, 1700).level);  // the breath
  TEST_ASSERT_TRUE(say(Line::ShuttingDown, 2400).level > 0.8f);        // "doooow"
  TEST_ASSERT_TRUE(say(Line::ShuttingDown, 2700).vowel == Vowel::Oo);  // winding down
  TEST_ASSERT_EQUAL_FLOAT(0.0f, say(Line::ShuttingDown, 3100).level);

  const uint32_t error[] = {460, 1060, 2140, 2220};
  const uint32_t abortCues[] = {520, 1350, 2050, 2500};
  const uint32_t dead[] = {1700, 3410};
  TEST_ASSERT_TRUE(fitsStock(Line::UhOhHuman, error, 4));
  TEST_ASSERT_TRUE(fitsStock(Line::FineAnyway, abortCues, 4));
  TEST_ASSERT_TRUE(fitsStock(Line::ShuttingDown, dead, 2));
  TEST_ASSERT_FALSE(fitsStock(Line::Count, dead, 2));
}

void test_bell_breathes_in_until_the_ding() {
  BellCelebration bell;
  bell.reset(0);
  const Expression start = bell.update(0, false);
  const Expression full = bell.update(900, false);
  TEST_ASSERT_TRUE(full.rise > start.rise + 2.5f);             // the face lifts
  TEST_ASSERT_TRUE(full.mouthOpenDown > start.mouthOpenDown + 1.5f);  // drawing in an "o"
  TEST_ASSERT_EQUAL_FLOAT(1.0f, full.mouthHollow);
  TEST_ASSERT_TRUE(full.pupil < start.pupil);                  // pupils tighten
  TEST_ASSERT_EQUAL_FLOAT(full.rise, bell.update(1300, false).rise);  // held to the ding
  for (uint32_t t = 0; t <= 1300; t += 100) {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, bell.update(t, false).quiver);  // no shaking yet
  }
}

void test_bell_yells_then_pupils_shake_with_the_ring() {
  BellCelebration bell;
  bell.reset(0);
  bell.update(500, false);
  const Expression yell = bell.update(1000, true);  // the ding
  TEST_ASSERT_TRUE(yell.mouthOpenDown > 6.0f);       // a big excited yell
  TEST_ASSERT_EQUAL_FLOAT(0.0f, yell.rise);           // the breath lets go
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, bell.ring(1040));  // strike peak

  const Expression strike = bell.update(1040, true);
  const Expression mid = bell.update(1700, true);
  const Expression tail = bell.update(2300, false);
  TEST_ASSERT_TRUE(strike.quiver > 1.9f);
  TEST_ASSERT_TRUE(mid.quiver < strike.quiver && mid.quiver > 0.5f);
  TEST_ASSERT_TRUE(tail.quiver > 0.0f && tail.quiver < mid.quiver);  // still at its last
  TEST_ASSERT_TRUE(tail.quiverHz < strike.quiverHz);                 // slowing down
  TEST_ASSERT_EQUAL_FLOAT(0.0f, bell.update(2400, false).quiver);    // the ring is gone
  TEST_ASSERT_EQUAL_FLOAT(0.0f, bell.ring(2400));
  TEST_ASSERT_TRUE(mid.happyLeft < 0.5f);                              // eyes open: pupils show
  TEST_ASSERT_EQUAL_FLOAT(
    expressionFor(Mood::Afterglow).lid, bell.update(3000, false).lid
  );
}

void test_bell_without_a_clip_still_dings() {
  BellCelebration bell;
  bell.reset(10000);
  TEST_ASSERT_FALSE(bell.update(11400, false).quiver > 0.0f);
  TEST_ASSERT_FALSE(bell.dinged());
  bell.update(11500, false);
  TEST_ASSERT_TRUE(bell.dinged());
  TEST_ASSERT_TRUE(bell.update(11540, false).quiver > 1.9f);
}

void test_only_the_pupils_shake() {
  static uint8_t still[kFrameBytes];
  Face f = plainFace();
  f.right.rx = 0.0f;
  render(f, still, kFrameBytes);
  float cx = 0.0f;
  float cy = 0.0f;
  memcpy(g_frame, still, kFrameBytes);
  pupilCentroid(cx, cy);

  f.left.pupilShake = 2.0f;
  render(f, g_frame, kFrameBytes);
  float x = 0.0f;
  float y = 0.0f;
  pupilCentroid(x, y);
  TEST_ASSERT_TRUE(x > cx + 1.0f);  // the pupil moved

  for (int row = 0; row < kHeight; row++) {  // the eye's outline did not
    int first = -1;
    int stillFirst = -1;
    for (int px = 0; px < 64; px++) {
      first = first < 0 && pixelAt(g_frame, px, row) ? px : first;
      stillFirst = stillFirst < 0 && pixelAt(still, px, row) ? px : stillFirst;
    }
    TEST_ASSERT_EQUAL_INT(stillFirst, first);
  }
}

void test_stars_fly_out_of_the_eyes() {
  Face f = plainFace();
  addStarBurst(f, 250, 0.5f);
  TEST_ASSERT_TRUE(f.sparkCount > 4);
  int left = 0;
  int right = 0;
  for (uint8_t i = 0; i < f.sparkCount; i++) {
    left += f.sparks[i].x < 32 - 11 ? 1 : 0;   // out past the left eye's outer side
    right += f.sparks[i].x > 96 + 11 ? 1 : 0;  // and the right eye's
  }
  TEST_ASSERT_EQUAL_INT(f.sparkCount, left + right);
  TEST_ASSERT_TRUE(left > 0 && right > 0);

  Face loud = plainFace();
  Face quiet = plainFace();
  addStarBurst(loud, 250, 1.0f);
  addStarBurst(quiet, 250, 0.05f);
  TEST_ASSERT_TRUE(loud.sparkCount > quiet.sparkCount);  // trails fade with the ring

  Face over = plainFace();
  addStarBurst(over, 1500, 0.0f);
  TEST_ASSERT_EQUAL_INT(0, over.sparkCount);  // gone with the ring

  render(f, g_frame, kFrameBytes);  // drawn without overflow
  TEST_ASSERT_TRUE(litCount(0, 0, 20, 31) > 0);
}

void test_shiver_changes_speed_without_jumping() {
  Animator a;
  Expression e = expressionFor(Mood::Bliss);
  a.snap(e);
  float prev = a.quiverPhase();
  for (int i = 0; i < 60; i++) {
    e.quiverHz = 14.0f - i * 0.2f;  // slowing down
    a.update(e, 33);
    float step = a.quiverPhase() - prev;
    step = step < 0.0f ? step + 6.2831853f : step;
    TEST_ASSERT_TRUE(step <= 6.2831853f * 14.0f * 0.033f + 0.001f);  // never skips ahead
    prev = a.quiverPhase();
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_render_rejects_small_buffer);
  RUN_TEST(test_render_is_pure);
  RUN_TEST(test_open_eye_has_a_pupil_and_catch_light);
  RUN_TEST(test_pupil_follows_the_gaze);
  RUN_TEST(test_pupil_keeps_its_size_while_it_moves);
  RUN_TEST(test_pupil_keeps_a_rim);
  RUN_TEST(test_typing_reads_lines_then_darts);
  RUN_TEST(test_full_smile_has_no_pupil);
  RUN_TEST(test_closed_eye_is_a_lid_line);
  RUN_TEST(test_happy_eye_is_a_crescent);
  RUN_TEST(test_lid_and_tilt_cut_the_top);
  RUN_TEST(test_blush_keeps_a_gap_from_the_eyes);
  RUN_TEST(test_hollow_mouth_is_an_outline);
  RUN_TEST(test_every_kind_draws_and_clips);
  RUN_TEST(test_mood_names_are_unique);
  RUN_TEST(test_shape_moods_use_their_kind);
  RUN_TEST(test_kind_swap_blinks_through);
  RUN_TEST(test_expression_eases_and_settles);
  RUN_TEST(test_long_gap_is_clamped);
  RUN_TEST(test_voice_opens_the_resting_mouth);
  RUN_TEST(test_babble_speaks_in_phrases);
  RUN_TEST(test_babble_survives_millis_wrap);
  RUN_TEST(test_follow_tracks_audio);
  RUN_TEST(test_mouth_line_ramps_between_keyframes);
  RUN_TEST(test_hello_human_says_the_words);
  RUN_TEST(test_psst_human_says_the_words);
  RUN_TEST(test_error_abort_dead_say_their_lines);
  RUN_TEST(test_bell_breathes_in_until_the_ding);
  RUN_TEST(test_bell_yells_then_pupils_shake_with_the_ring);
  RUN_TEST(test_bell_without_a_clip_still_dings);
  RUN_TEST(test_only_the_pupils_shake);
  RUN_TEST(test_stars_fly_out_of_the_eyes);
  RUN_TEST(test_shiver_changes_speed_without_jumping);
  RUN_TEST(test_idle_is_mostly_smiles_with_accents);
  RUN_TEST(test_idle_survives_millis_wrap);
  RUN_TEST(test_working_moods_are_mostly_focused_with_positive_accents);
  RUN_TEST(test_work_going_well_brings_more_smiles);
  return UNITY_END();
}
