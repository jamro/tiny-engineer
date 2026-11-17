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
  // Sets the firmware's angle without writing the chip; boot writes every
  // channel afterwards.
  void restorePosition(float angle);
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
  // False when the write failed or did not read back.
  bool resyncOutput();

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

// Reads the PCA9685's setup registers. True when it answers, is awake with
// auto-increment on, and holds the servo output mode and prescale.
bool servoPwmDriverState(ServoPwmDriverState& state);
// Bringing the outputs back, in steps a caller can spread over several loop
// passes: setUpServoPwmDriver() if the setup was lost, writeServoChannelsOff(),
// at least kServoOutputSettleMs, then enableServoOutputsWithChannels(). Each
// returns false, with the outputs off, when a write failed or did not read
// back.
constexpr uint32_t kServoOutputSettleMs = 25;  // more than one 20 ms period
// Wakes the PCA9685 and writes its output mode and frequency, outputs off.
// True when the setup reads back.
bool setUpServoPwmDriver();
// Turns every servo channel fully off, outputs off.
bool writeServoChannelsOff();
// Turns the outputs on, then writes every channel from the firmware's state;
// keeps the current pose.
bool enableServoOutputsWithChannels();
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
