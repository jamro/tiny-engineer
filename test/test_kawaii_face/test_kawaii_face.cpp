#include <cstring>
#include <unity.h>

#include "display/eyes/styles/kawaii/face.h"
#include "display/eyes/styles/kawaii/mood.h"

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

void test_open_eye_has_body_and_sparkle() {
  render(plainFace(), g_frame, kFrameBytes);

  TEST_ASSERT_TRUE(pixelAt(g_frame, 33, 18));   // body, right of center
  TEST_ASSERT_FALSE(pixelAt(g_frame, 28, 12));  // big highlight, upper left
  TEST_ASSERT_FALSE(pixelAt(g_frame, 32, 2));   // above the eye
  TEST_ASSERT_FALSE(pixelAt(g_frame, 64, 16));  // between the eyes
  TEST_ASSERT_TRUE(pixelAt(g_frame, 97, 18));
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

void test_speech_opens_the_mouth() {
  Expression quiet = expressionFor(Mood::Talking);
  Expression loud = quiet;
  applySpeech(quiet, 0.05f, true);
  applySpeech(loud, 0.9f, true);
  TEST_ASSERT_TRUE(loud.mouthOpenDown > quiet.mouthOpenDown + 2.0f);

  Expression clamped = expressionFor(Mood::Talking);
  applySpeech(clamped, 5.0f, true);
  TEST_ASSERT_TRUE(clamped.mouthOpenDown <= 0.6f + 4.5f + 0.001f);
}

void test_idle_is_mostly_smiles_with_accents() {
  IdleDirector idle(testRand);
  idle.reset(0);

  uint32_t smileMs = 0;
  uint32_t totalMs = 0;
  bool sawAccent[(size_t)Mood::Count] = {};

  for (uint32_t now = 0; now < 10u * 60u * 1000u; now += 33) {
    const Mood m = idle.update(now);
    TEST_ASSERT_TRUE(m <= Mood::Love);  // idle family only
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
  IdleDirector idle(testRand);
  const uint32_t start = 0xFFFFFFFFu - 20000u;
  idle.reset(start);

  bool sawAccent = false;
  for (uint32_t t = 0; t < 60000u; t += 33) {
    const Mood m = idle.update(start + t);
    sawAccent = sawAccent || (m != Mood::Content && m != Mood::Smile);
  }
  TEST_ASSERT_TRUE(sawAccent);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_render_rejects_small_buffer);
  RUN_TEST(test_open_eye_has_body_and_sparkle);
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
  RUN_TEST(test_speech_opens_the_mouth);
  RUN_TEST(test_idle_is_mostly_smiles_with_accents);
  RUN_TEST(test_idle_survives_millis_wrap);
  return UNITY_END();
}
