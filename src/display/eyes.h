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
  Ring,
  Welcome,
  Attention,
  Error,
  Abort,
  Wakeup,
  Dead
};

enum class SleepEyeResult {
  Running,
  CloseComplete,
  OpenComplete
};

void startEyes();
void startEyesForWake(uint32_t now);
void stopEyes();
void updateEyes(uint32_t now);
bool eyesActive();

void requestSleepEyeClose(uint32_t now);
void requestSleepEyeOpen(uint32_t now);
void clearSleepEyeAnim();
SleepEyeResult updateSleepEyes(uint32_t now);

void setEyeMode(EyeMode mode, uint32_t now);
void triggerEyeImpact(uint32_t now);

const Eye& leftEye();
const Eye& rightEye();
Eye& mutableLeftEye();
Eye& mutableRightEye();
