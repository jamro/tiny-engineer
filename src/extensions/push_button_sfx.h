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

// Start unless none, /play busy, or a sound-owning anim is streaming.
// Orphan SFX may be preempted by wavStreamStart.
inline bool shouldStartButtonSfx(
  ButtonSfxClip clip,
  bool apiBusy,
  bool animOwnsActiveStream
) {
  return clip != ButtonSfxClip::None && !apiBusy && !animOwnsActiveStream;
}
