#pragma once

void initPca9685();
void setAllServoAngles(float angle);
void moveAllServosSmooth(
  float fromAngle,
  float toAngle,
  int durationMs
);
void runServoTest();
