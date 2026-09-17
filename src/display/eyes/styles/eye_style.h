#pragma once

#include <cstddef>
#include <cstdint>

#include "display/eyes.h"
#include "animation.h"

// Sleep lid phase driven by the eyes controller; styles may ignore it.
enum class EyeStyleSleepPhase {
  None,
  Closing,
  Opening
};

struct EyeStyleRenderer {
  const char* id;
  bool needsPoseUpdate;
  bool needsBlink;
  void (*draw)(
    EyeMode mode,
    AnimationId animation,
    uint32_t modeStartedMs,
    uint32_t now,
    EyeStyleSleepPhase sleepPhase
  );
};

// Registered style string ids (classic, kaomoji, …). Safe for host validate.
bool isRegisteredEyeStyleId(const char* id);
size_t registeredEyeStyleCount();
const char* registeredEyeStyleIdAt(size_t index);

// Full renderer lookup for firmware. Unknown id → classic.
const EyeStyleRenderer* findEyeStyle(const char* id);
const EyeStyleRenderer* currentEyeStyle();
