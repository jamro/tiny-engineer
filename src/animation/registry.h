#pragma once

#include <cstdint>

#include "animation.h"
#include "display/eyes.h"

struct ModeEntry {
  AnimationId animId;
  EyeMode eyeMode;
  const char* name;
  bool continuous;
  int32_t blinkHoldMs;  // -1 = no special blink; else blinkBeginIdle + nextBlink
  void (*startAnim)(uint32_t nowMs);
  void (*updateAnim)(uint32_t nowMs);
  void (*startEyes)(uint32_t nowMs);
  void (*updateEyes)(uint32_t nowMs);
};

const ModeEntry* modeByAnimId(AnimationId id);
const ModeEntry* modeByEyeMode(EyeMode mode);
const ModeEntry* modeByName(const char* name);
