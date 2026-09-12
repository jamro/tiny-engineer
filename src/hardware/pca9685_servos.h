#pragma once

void initPca9685();
bool moveServoSmooth(int index, float toAngle);
bool moveServoSmoothElectrical(int index, float toAngle);
void servoMoveAllToElectricalAngle(float angle);
void centerAllServos();
void runServoTest();
