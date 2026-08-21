#pragma once

#include <cstdint>

extern bool oledAvailable;

bool i2cDeviceConnected(uint8_t address);

void showOledText(
  const char* line1,
  const char* line2 = nullptr
);

void initOled();
void runOledTest();

void showServoProgress(
  int step,
  int totalSteps,
  const char* action
);

void showServoTestFinished();
