#pragma once

enum class AnimationId {
  None,
  Typing,
  Reading,
  Thinking,
  Ring,
  Welcome,
  Attention
};

void setAnimation(AnimationId id);
void finishAnimation();
AnimationId getAnimation();
const char* animationName(AnimationId id);
bool parseAnimationName(const char* name, AnimationId& out);
void updateAnimation();
