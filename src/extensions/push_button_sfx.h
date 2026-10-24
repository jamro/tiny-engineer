#pragma once

#include <cstdint>
#include <cstring>

enum class ButtonSfxClip : uint8_t {
  None,
  Bell,
  Click,
};

inline ButtonSfxClip parseButtonSfx(const char* sound) {
  if (sound == nullptr || strcmp(sound, "none") == 0) {
    return ButtonSfxClip::None;
  }
  if (strcmp(sound, "bell") == 0) {
    return ButtonSfxClip::Bell;
  }
  if (strcmp(sound, "click") == 0) {
    return ButtonSfxClip::Click;
  }
  return ButtonSfxClip::None;
}

// True when a mapped clip should start (not none, speaker free).
inline bool shouldStartButtonSfx(ButtonSfxClip clip, bool streamActive, bool apiBusy) {
  return clip != ButtonSfxClip::None && !streamActive && !apiBusy;
}
