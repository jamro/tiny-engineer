#include <unity.h>

#include <cmath>
#include <vector>

#include "audio/audio_motion.h"
#include "pins.h"

namespace {

constexpr size_t kBatch = 256;

std::vector<int16_t> tone(int16_t amplitude) {
  std::vector<int16_t> samples(kBatch);
  for (size_t i = 0; i < kBatch; i++) {
    // Alternate sign so mean is ~0 but mean-abs is amplitude.
    samples[i] = (i & 1) ? -amplitude : amplitude;
  }
  return samples;
}

std::vector<int16_t> silence() {
  return std::vector<int16_t>(kBatch, 0);
}

uint32_t advanceMs(uint32_t& t, size_t samples) {
  t += static_cast<uint32_t>(samples * 1000ULL / SAMPLE_RATE);
  if ((samples * 1000ULL / SAMPLE_RATE) == 0) {
    t += 1;
  }
  return t;
}

void feedBatches(
  const std::vector<int16_t>& batch,
  int count,
  uint32_t& t
) {
  for (int i = 0; i < count; i++) {
    advanceMs(t, batch.size());
    audioMotionFeed(batch.data(), batch.size(), t);
  }
}

}  // namespace

void test_idle_snapshot_inactive() {
  audioMotionEnd(0);
  const AudioMotionSnapshot snap = audioMotionSnapshot(0);
  TEST_ASSERT_FALSE(snap.streamActive);
  TEST_ASSERT_FALSE(snap.speaking);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, snap.level);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, snap.emphasis);
}

void test_silence_not_speaking() {
  uint32_t t = 1000;
  audioMotionBegin(t);
  feedBatches(silence(), 20, t);

  const AudioMotionSnapshot snap = audioMotionSnapshot(t);
  TEST_ASSERT_TRUE(snap.streamActive);
  TEST_ASSERT_FALSE(snap.speaking);
  TEST_ASSERT_TRUE(snap.level < 0.05f);
  TEST_ASSERT_EQUAL_UINT32(0, snap.phraseId);
  audioMotionEnd(t);
}

void test_sustained_tone_speaking() {
  uint32_t t = 2000;
  audioMotionBegin(t);
  feedBatches(tone(8000), 30, t);

  const AudioMotionSnapshot snap = audioMotionSnapshot(t);
  TEST_ASSERT_TRUE(snap.streamActive);
  TEST_ASSERT_TRUE(snap.speaking);
  TEST_ASSERT_TRUE(snap.level > 0.3f);
  TEST_ASSERT_TRUE(snap.phraseId >= 1);
  audioMotionEnd(t);
}

void test_onset_raises_emphasis() {
  uint32_t t = 3000;
  audioMotionBegin(t);
  feedBatches(silence(), 10, t);
  const float before = audioMotionSnapshot(t).emphasis;

  feedBatches(tone(12000), 3, t);
  const AudioMotionSnapshot snap = audioMotionSnapshot(t);
  TEST_ASSERT_TRUE(snap.emphasis > before);
  TEST_ASSERT_TRUE(snap.emphasis > 0.05f);
  audioMotionEnd(t);
}

void test_phrase_boundary_after_silence() {
  uint32_t t = 4000;
  audioMotionBegin(t);
  feedBatches(tone(8000), 20, t);
  const uint32_t firstPhrase = audioMotionSnapshot(t).phraseId;
  TEST_ASSERT_EQUAL_UINT32(1, firstPhrase);

  // Gate release (~70 ms) + phrase silence (≥250 ms): ~40 × 11.6 ms batches.
  feedBatches(silence(), 40, t);
  TEST_ASSERT_FALSE(audioMotionSnapshot(t).speaking);

  feedBatches(tone(8000), 20, t);
  const AudioMotionSnapshot snap = audioMotionSnapshot(t);
  TEST_ASSERT_TRUE(snap.speaking);
  TEST_ASSERT_EQUAL_UINT32(2, snap.phraseId);
  audioMotionEnd(t);
}

void test_end_clears_stream() {
  uint32_t t = 5000;
  audioMotionBegin(t);
  feedBatches(tone(8000), 10, t);
  audioMotionEnd(t);

  const AudioMotionSnapshot snap = audioMotionSnapshot(t);
  TEST_ASSERT_FALSE(snap.streamActive);
  TEST_ASSERT_FALSE(snap.speaking);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, snap.level);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_idle_snapshot_inactive);
  RUN_TEST(test_silence_not_speaking);
  RUN_TEST(test_sustained_tone_speaking);
  RUN_TEST(test_onset_raises_emphasis);
  RUN_TEST(test_phrase_boundary_after_silence);
  RUN_TEST(test_end_clears_stream);
  return UNITY_END();
}
