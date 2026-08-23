#pragma once

#include <stdint.h>

#include "servos.h"

class ServoWrapper {
public:
  explicit ServoWrapper(int index);

  float angle() const;
  bool moveTo(float target);
  void snapTo(float angle);

  void setTarget(
    float target,
    float speedDegS = SERVO_MAX_SPEED_DEG_S
  );
  void setPosition(float angle);
  void stop();
  void update();
  bool isMoving() const;

  friend void servoMoveAllSmooth(float toAngle);

private:
  int index_;
  float angle_;
  float target_;
  float speedDegS_;
  uint32_t lastUpdateMs_;
  uint16_t lastPulse_;

  void writeAngle(float angle, bool log);
};

ServoWrapper& servoAt(int index);
void initServoPwmDriver();
void servoMoveAllSmooth(float toAngle);
void updateAllServos();
