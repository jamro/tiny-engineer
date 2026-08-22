#pragma once

enum class AnimationId {
  None,
  Typing,
  Reading,
  Thinking,
  Ring,
  Welcome,
  Attention,
  Error
};

void setAnimation(AnimationId id);
void finishAnimation(uint32_t nowMs);
AnimationId getAnimation();
const char* animationName(AnimationId id);
bool parseAnimationName(const char* name, AnimationId& out);
void updateAnimation();
