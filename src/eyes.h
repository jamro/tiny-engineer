#pragma once

#include <cstdint>

struct Eye {
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
};

void startEyes();
void stopEyes();
void updateEyes(uint32_t now);
bool eyesActive();

const Eye& leftEye();
const Eye& rightEye();
Eye& mutableLeftEye();
Eye& mutableRightEye();
