#pragma once

#include <cstdint>

struct Eye;

extern bool oledAvailable;

bool i2cDeviceConnected(uint8_t address);

void showOledText(
  const char* line1,
  const char* line2 = nullptr
);

void showBootProgress(
  int step,
  int totalSteps,
  const char* label
);

void showBootIp(const char* ip);

void showIdleScreen();

void drawEyes(
  const Eye& left,
  const Eye& right,
  int cornerRadius
);

void initOled();
void runOledTest();

void showServoProgress(
  int step,
  int totalSteps,
  const char* action
);

void showServoTestFinished();

void blankOled();
void sleepOled();
void wakeOled();
