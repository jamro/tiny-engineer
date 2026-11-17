#pragma once

#include <cstdint>

extern bool oledAvailable;

bool i2cDeviceConnected(uint8_t address);

void showOledText(
  const char* line1,
  const char* line2 = nullptr
);

void showProvisioningOled(
  const char* line1,
  const char* line2
);

void updateProvisioningOled(uint32_t nowMs);

void showBootProgress(
  int step,
  int totalSteps,
  const char* label
);

void showBootIp(const char* ip);

// Brief status (IP / two-line text), then idle eyes. Shared by boot and
// in-session setup-AP leave so status text does not stick forever.
void showBootIpThenIdle(const char* ip);
void showTextThenIdle(const char* line1, const char* line2 = nullptr);

void showIdleScreen();

void initOled();
// True when the OLED answered at boot and its driver started, even if its
// setup then failed: the I2C checks watch it and retry that setup.
bool oledDetectedAtBoot();
// Sets the OLED up again after an I2C fault and sends the last frame, so a
// static screen survives too. Checks every transfer; false leaves the OLED
// unavailable. With panelOn false the panel stays dark but holds the frame for
// when it wakes. Keeps the current rotation.
bool restartOled(bool panelOn);
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
void applyOledRotation();
void showOledOrientationTest(bool rotate180);
void restoreProvisioningOled();
