#pragma once

#include <stdint.h>

#include "pins.h"
#include "servos.h"

class ServoWrapper {
public:
  explicit ServoWrapper(int index);

  float angle() const;
  bool moveTo(float target);
  bool moveToElectrical(float target, float speedDegS);
  void snapTo(float angle);

  void setTarget(
    float target,
    float speedDegS = SERVO_MAX_SPEED_DEG_S
  );
  void setPosition(float angle);
  void setNormTarget(
    float n,
    float speedDegS = SERVO_MAX_SPEED_DEG_S
  );
  void setNormPosition(float n);
  void stop();
  void update();
  bool isMoving() const;
  void ensureServoOutput();
  void releaseServoOutput();
  // Rewrites this channel from the firmware's state after a driver restart.
  void resyncOutput();

  friend void servoMoveAllSmoothTo(
    const float targets[SERVO_COUNT],
    float speedDegS
  );
  friend void servoMoveAllToElectrical(
    const float targets[SERVO_COUNT],
    float speedDegS
  );

private:
  int index_;
  float angle_;
  float target_;
  float speedDegS_;
  uint32_t lastUpdateMs_;
  uint16_t lastPulse_;
  bool released_;

  void writeAngle(float angle, bool log, bool electrical);
};

ServoWrapper& servoAt(int index);
void initServoOutputPin();
void disableServoOutputs();
void enableServoOutputs();
void initServoPwmDriver();
struct ServoPwmDriverState {
  bool answered;
  uint8_t mode1;
  uint8_t mode2;
  uint8_t prescale;
};

// Reads the PCA9685's setup registers. True when it answers, is awake, and
// still holds the output mode and prescale it was given.
bool servoPwmDriverState(ServoPwmDriverState& state);
bool servoPwmDriverResponds();
// Sets the PCA9685 up again and rewrites every channel; keeps the current pose.
void restartServoPwmDriver();
void servoMoveAllSmoothTo(
  const float targets[SERVO_COUNT],
  float speedDegS = SERVO_BOOT_SPEED_DEG_S
);
void servoMoveAllToElectrical(
  const float targets[SERVO_COUNT],
  float speedDegS = SERVO_CALIB_SPEED_DEG_S
);
void updateAllServos();
bool anyServoMoving();
void ensureAllServoOutputs();
void releaseAllServoOutputs();

// Persist last commanded angles; true when saved or already up to date.
bool flushServoPoseToNvs();
void pollServoPoseSave();
void setServoParkHoldUntilReboot(bool hold);
bool servoOutputsHoldActive();

float clampElectricalAngle(float angle);
float clampServoAngle(int index, float angle);
float servoRuntimeMid(int index);
float servoNormToDeg(int index, float n);
float servoDegToNorm(int index, float deg);
