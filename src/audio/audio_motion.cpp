#include "audio/audio_motion.h"

#include <cmath>

#include "pins.h"

namespace {

constexpr float kSpeakOn = 0.12f;
constexpr float kSpeakOff = 0.06f;
constexpr float kAttackTauMs = 25.0f;
constexpr float kReleaseTauMs = 180.0f;
constexpr float kGateReleaseTauMs = 70.0f;
constexpr float kPeakTauMs = 800.0f;
constexpr float kEmphasisRise = 0.08f;
constexpr float kEmphasisDecayMs = 100.0f;
constexpr uint32_t kPhraseSilenceMs = 250;
constexpr float kMinPeak = 400.0f;  // int16 mean-abs floor so silence stays quiet

bool g_streamActive = false;
bool g_speaking = false;
float g_level = 0.0f;
float g_gateLevel = 0.0f;
float g_emphasis = 0.0f;
float g_peak = kMinPeak;
uint32_t g_phraseId = 0;
uint32_t g_lastFeedMs = 0;
uint32_t g_silenceSinceMs = 0;
bool g_haveSilenceClock = false;

float clamp01(float v) {
  if (v < 0.0f) {
    return 0.0f;
  }
  if (v > 1.0f) {
    return 1.0f;
  }
  return v;
}

float meanAbs(const int16_t* samples, size_t count) {
  if (count == 0) {
    return 0.0f;
  }

  int64_t sum = 0;
  for (size_t i = 0; i < count; i++) {
    const int32_t s = samples[i];
    sum += s < 0 ? -s : s;
  }
  return static_cast<float>(sum) / static_cast<float>(count);
}

float smoothToward(float current, float target, float tauMs, float dtMs) {
  if (dtMs <= 0.0f || tauMs <= 0.0f) {
    return target;
  }
  const float a = 1.0f - std::exp(-dtMs / tauMs);
  return current + (target - current) * a;
}

void resetFeatures(uint32_t nowMs) {
  g_speaking = false;
  g_level = 0.0f;
  g_gateLevel = 0.0f;
  g_emphasis = 0.0f;
  g_peak = kMinPeak;
  g_lastFeedMs = nowMs;
  g_silenceSinceMs = nowMs;
  g_haveSilenceClock = true;
}

}  // namespace

void audioMotionBegin(uint32_t nowMs) {
  g_streamActive = true;
  g_phraseId = 0;
  resetFeatures(nowMs);
}

void audioMotionEnd(uint32_t /*nowMs*/) {
  g_streamActive = false;
  g_speaking = false;
  g_level = 0.0f;
  g_gateLevel = 0.0f;
  g_emphasis = 0.0f;
}

void audioMotionFeed(const int16_t* samples, size_t count, uint32_t nowMs) {
  if (!g_streamActive || samples == nullptr || count == 0) {
    return;
  }

  float dtMs = 0.0f;
  if (g_lastFeedMs != 0 && nowMs >= g_lastFeedMs) {
    dtMs = static_cast<float>(nowMs - g_lastFeedMs);
  }
  // Prefer sample-count timing when wall clock stalls (same millis() across batches).
  const float batchMs =
    static_cast<float>(count) * 1000.0f / static_cast<float>(SAMPLE_RATE);
  if (dtMs < batchMs * 0.5f) {
    dtMs = batchMs;
  }
  g_lastFeedMs = nowMs;

  const float raw = meanAbs(samples, count);
  if (raw > g_peak) {
    g_peak = smoothToward(g_peak, raw, kAttackTauMs, dtMs);
  } else {
    g_peak = smoothToward(g_peak, raw, kPeakTauMs, dtMs);
  }
  if (g_peak < kMinPeak) {
    g_peak = kMinPeak;
  }

  const float instant = clamp01(raw / g_peak);
  const float prevLevel = g_level;
  if (instant > g_level) {
    g_level = smoothToward(g_level, instant, kAttackTauMs, dtMs);
  } else {
    g_level = smoothToward(g_level, instant, kReleaseTauMs, dtMs);
  }

  // Faster gate for speak/silence so phrase pauses register without waiting on motion release.
  if (instant > g_gateLevel) {
    g_gateLevel = smoothToward(g_gateLevel, instant, kAttackTauMs, dtMs);
  } else {
    g_gateLevel = smoothToward(g_gateLevel, instant, kGateReleaseTauMs, dtMs);
  }

  const float rise = g_level - prevLevel;
  if (rise >= kEmphasisRise) {
    g_emphasis = clamp01(g_emphasis + rise * 4.0f);
  } else {
    g_emphasis = smoothToward(g_emphasis, 0.0f, kEmphasisDecayMs, dtMs);
  }

  const bool wasSpeaking = g_speaking;
  if (g_speaking) {
    if (g_gateLevel < kSpeakOff) {
      g_speaking = false;
    }
  } else if (g_gateLevel >= kSpeakOn) {
    g_speaking = true;
  }

  if (g_speaking) {
    if (!wasSpeaking) {
      uint32_t silenceMs = 0;
      if (g_haveSilenceClock && nowMs >= g_silenceSinceMs) {
        silenceMs = nowMs - g_silenceSinceMs;
      }
      // First phrase, or resume after a pause.
      if (g_phraseId == 0 || silenceMs >= kPhraseSilenceMs) {
        g_phraseId++;
      }
    }
    g_haveSilenceClock = false;
  } else if (!g_haveSilenceClock) {
    g_silenceSinceMs = nowMs;
    g_haveSilenceClock = true;
  }
}

AudioMotionSnapshot audioMotionSnapshot(uint32_t /*nowMs*/) {
  AudioMotionSnapshot snap;
  snap.streamActive = g_streamActive;
  snap.speaking = g_streamActive && g_speaking;
  snap.level = g_streamActive ? g_level : 0.0f;
  snap.emphasis = g_streamActive ? g_emphasis : 0.0f;
  snap.phraseId = g_phraseId;
  return snap;
}
