#pragma once

enum class AnimationId {
  None,
  Typing,
  Reading,
  Thinking
};

void setAnimation(AnimationId id);
AnimationId getAnimation();
const char* animationName(AnimationId id);
bool parseAnimationName(const char* name, AnimationId& out);
void updateAnimation();
