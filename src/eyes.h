#pragma once

#include <cstdint>

struct Eye {
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
};

enum class EyeMode {
  Idle,
  Typing,
  Reading,
  Thinking,
  Ring
};

void startEyes();
void stopEyes();
void updateEyes(uint32_t now);
bool eyesActive();

void setEyeMode(EyeMode mode, uint32_t now);
void triggerEyeImpact(uint32_t now);

const Eye& leftEye();
const Eye& rightEye();
Eye& mutableLeftEye();
Eye& mutableRightEye();
