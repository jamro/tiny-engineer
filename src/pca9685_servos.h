#pragma once

void initPca9685();
bool moveServoSmooth(int index, float toAngle);
void centerAllServos();
void runServoTest();
