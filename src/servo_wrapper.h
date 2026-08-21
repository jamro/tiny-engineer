#pragma once

#include <stdint.h>

class ServoWrapper {
public:
  explicit ServoWrapper(int index);

  float angle() const;
  bool moveTo(float target);
  void snapTo(float angle);

  void setTarget(float target);
  void stop();
  void update();
  bool isMoving() const;

  friend void servoMoveAllSmooth(float toAngle);

private:
  int index_;
  float angle_;
  float target_;
  uint32_t lastUpdateMs_;

  void writeAngle(float angle, bool log);
};

ServoWrapper& servoAt(int index);
void initServoPwmDriver();
void servoMoveAllSmooth(float toAngle);
void updateAllServos();
