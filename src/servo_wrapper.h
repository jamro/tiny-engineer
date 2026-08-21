#pragma once

class ServoWrapper {
public:
  explicit ServoWrapper(int index);

  float angle() const;
  bool moveTo(float target);
  void snapTo(float angle);

  friend void servoMoveAllSmooth(float toAngle);

private:
  int index_;
  float angle_;

  void writeAngle(float angle, bool log);
};

ServoWrapper& servoAt(int index);
void initServoPwmDriver();
void servoMoveAllSmooth(float toAngle);
