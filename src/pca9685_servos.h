#pragma once

void initPca9685();
bool setServoAngle(int index, float angle);
bool moveServoSmooth(int index, float toAngle);
void setAllServoAngles(float angle);
void centerAllServos();
void moveAllServosSmooth(
  float fromAngle,
  float toAngle,
  int durationMs
);
void runServoTest();
