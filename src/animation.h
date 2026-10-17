#pragma once

#include <cstdint>

enum class AnimationId {
  None,
  Typing,
  Reading,
  Thinking,
  Ring,
  Welcome,
  Attention,
  Error,
  Abort,
  Wakeup,
  Sleep,
  Dead,
  Talking
};

void setAnimation(AnimationId id);
// Skips the minimum hold and restarts `id` even when it is already running.
void setAnimationImmediately(AnimationId id);
void finishAnimation(uint32_t nowMs);
AnimationId getAnimation();
bool hasPendingAnimation();
AnimationId pendingAnimation();
bool animationIsContinuous(AnimationId id);
const char* animationName(AnimationId id);
bool parseAnimationName(const char* name, AnimationId& out);
void updateAnimation();
