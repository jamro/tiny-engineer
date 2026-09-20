#pragma once

#include <stdint.h>

enum class WavClip : uint8_t {
  Bell,
  Welcome,
  Attention,
  Error,
  Abort,
  Dead,
};

constexpr uint8_t kWavClipCount = 6;

const char* wavClipPath(WavClip clip);
const char* wavClipOledTitle(WavClip clip);

bool wavStreamStart(WavClip clip);
bool wavStreamUpdate();
void wavStreamStop();
bool wavStreamIsPlaying(WavClip clip);
