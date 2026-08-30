#include "animation/registry.h"

#include <cstddef>
#include <cstring>

#include "animation/abort.h"
#include "animation/attention.h"
#include "animation/dead.h"
#include "animation/error.h"
#include "animation/reading.h"
#include "animation/ring.h"
#include "animation/sleep_anim.h"
#include "animation/thinking.h"
#include "animation/typing.h"
#include "animation/util.h"
#include "animation/wakeup.h"
#include "animation/welcome.h"
#include "display/eyes/modes/abort.h"
#include "display/eyes/modes/attention.h"
#include "display/eyes/modes/dead.h"
#include "display/eyes/modes/error.h"
#include "display/eyes/modes/idle.h"
#include "display/eyes/modes/reading.h"
#include "display/eyes/modes/ring.h"
#include "display/eyes/modes/thinking.h"
#include "display/eyes/modes/typing.h"
#include "display/eyes/modes/wakeup.h"
#include "display/eyes/modes/welcome.h"
#include "sleep.h"

namespace {

void startNoneAt(uint32_t /*nowMs*/) {
  anim::stopAnimServos();
  // Sleeping: keep chin-down (same pose wakeup starts from).
  if (isSleeping()) {
    anim::parkSleepPose();
    return;
  }

  anim::parkNonePose();
}

void startTypingAt(uint32_t /*nowMs*/) {
  startTyping();
}

void startReadingAt(uint32_t /*nowMs*/) {
  startReading();
}

void startRingAt(uint32_t /*nowMs*/) {
  startRing();
}

void startWelcomeAt(uint32_t /*nowMs*/) {
  startWelcome();
}

void startAttentionAt(uint32_t /*nowMs*/) {
  startAttention();
}

void startErrorAt(uint32_t /*nowMs*/) {
  startError();
}

void startAbortAt(uint32_t /*nowMs*/) {
  startAbort();
}

void startWakeupAt(uint32_t nowMs) {
  startWakeup(nowMs, true);
}

void updateWakeupAt(uint32_t nowMs) {
  if (wakeupFinished()) {
    return;
  }

  updateWakeup(nowMs);

  if (wakeupFinished()) {
    finishAnimation(nowMs);
  }
}

void startSleepAt(uint32_t /*nowMs*/) {
  startSleepAnim();
}

void startDeadAt(uint32_t /*nowMs*/) {
  startDead();
}

constexpr ModeEntry kModes[] = {
  {
    AnimationId::None,
    EyeMode::Idle,
    "none",
    false,
    -1,
    startNoneAt,
    nullptr,
    startIdleEyes,
    updateIdleEyes,
  },
  {
    AnimationId::Typing,
    EyeMode::Typing,
    "typing",
    true,
    -1,
    startTypingAt,
    updateTyping,
    startTypingEyes,
    updateTypingEyes,
  },
  {
    AnimationId::Reading,
    EyeMode::Reading,
    "reading",
    true,
    -1,
    startReadingAt,
    updateReading,
    startReadingEyes,
    updateReadingEyes,
  },
  {
    AnimationId::Thinking,
    EyeMode::Thinking,
    "thinking",
    true,
    -1,
    startThinking,
    updateThinking,
    startThinkingEyes,
    updateThinkingEyes,
  },
  {
    AnimationId::Ring,
    EyeMode::Ring,
    "ring",
    false,
    -1,
    startRingAt,
    updateRing,
    startRingEyes,
    updateRingEyes,
  },
  {
    AnimationId::Welcome,
    EyeMode::Welcome,
    "welcome",
    false,
    10000,
    startWelcomeAt,
    updateWelcome,
    startWelcomeEyes,
    updateWelcomeEyes,
  },
  {
    AnimationId::Attention,
    EyeMode::Attention,
    "attention",
    false,
    1200,
    startAttentionAt,
    updateAttention,
    startAttentionEyes,
    updateAttentionEyes,
  },
  {
    AnimationId::Error,
    EyeMode::Error,
    "error",
    false,
    2300,
    startErrorAt,
    updateError,
    startErrorEyes,
    updateErrorEyes,
  },
  {
    AnimationId::Abort,
    EyeMode::Abort,
    "abort",
    false,
    1500,
    startAbortAt,
    updateAbort,
    startAbortEyes,
    updateAbortEyes,
  },
  {
    AnimationId::Wakeup,
    EyeMode::Wakeup,
    "wakeup",
    false,
    -1,
    startWakeupAt,
    updateWakeupAt,
    startWakeupEyes,
    updateWakeupEyes,
  },
  {
    AnimationId::Sleep,
    EyeMode::Idle,
    "sleep",
    false,
    -1,
    startSleepAt,
    updateSleepAnim,
    startIdleEyes,
    updateIdleEyes,
  },
  {
    AnimationId::Dead,
    EyeMode::Dead,
    "dead",
    false,
    -1,
    startDeadAt,
    updateDead,
    startDeadEyes,
    updateDeadEyes,
  },
};

constexpr size_t kModeCount = sizeof(kModes) / sizeof(kModes[0]);

}  // namespace

const ModeEntry* modeByAnimId(AnimationId id) {
  for (size_t i = 0; i < kModeCount; i++) {
    if (kModes[i].animId == id) {
      return &kModes[i];
    }
  }

  return &kModes[0];
}

const ModeEntry* modeByEyeMode(EyeMode mode) {
  for (size_t i = 0; i < kModeCount; i++) {
    if (kModes[i].eyeMode == mode) {
      return &kModes[i];
    }
  }

  return &kModes[0];
}

const ModeEntry* modeByName(const char* name) {
  if (name == nullptr) {
    return nullptr;
  }

  for (size_t i = 0; i < kModeCount; i++) {
    if (strcmp(kModes[i].name, name) == 0) {
      return &kModes[i];
    }
  }

  return nullptr;
}
