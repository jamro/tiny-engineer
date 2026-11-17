#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Preferences.h>
#include <cmath>
#include <cstring>

#include "animation/util.h"
#include "pins.h"
#include "servos.h"
#include "hardware/i2c_health.h"
#include "hardware/servo_wrapper.h"
#include "serial_log.h"
#include "settings/settings.h"

Adafruit_PWMServoDriver pwm(PCA9685_ADDRESS);

// PCA9685 LEDn_OFF full-off bit. Do not use (4096, 0) — that is full-on HIGH.
constexpr uint16_t kPca9685FullOff = 4096;
constexpr const char* kPoseNs = "te";
constexpr const char* kPoseKey = "spose";
constexpr uint32_t kPoseSaveDebounceMs = 500;

constexpr uint16_t kServoPwmHz = 50;

constexpr uint8_t kPcaMode1Reg = 0x00;
constexpr uint8_t kPcaMode2Reg = 0x01;
// Awake, on the internal clock, with register auto-increment on: setPWM
// writes four registers in one transfer and needs it. EXTCLK is sticky (only a
// power cycle clears it), so a corrupted one keeps the chip down, visibly.
constexpr uint8_t kPcaMode1Mask = MODE1_SLEEP | MODE1_AI | MODE1_EXTCLK;
constexpr uint8_t kPcaMode1Servo = MODE1_AI;
// Totem-pole outputs, not inverted: what a servo signal line needs. Any other
// value leaves the servos without a usable pulse while every write still
// succeeds.
constexpr uint8_t kPcaMode2Servo = MODE2_OUTDRV;
// What setPWMFreq(kServoPwmHz) programs, by the Adafruit driver's own formula.
// Computed, not read back: a readback after a lost write would bless the
// wrong value. A chip that lost power comes back with a different default.
constexpr uint8_t kPcaPrescaleServo =
  static_cast<uint8_t>(FREQUENCY_OSCILLATOR / (kServoPwmHz * 4096.0) + 0.5 - 1);

static bool g_allOutputsReleased = false;
static bool g_parkHoldUntilReboot = false;
static bool g_poseDirty = false;
static uint32_t g_poseDirtySinceMs = 0;
static uint8_t g_lastSavedPose[SERVO_COUNT] = {};
static bool g_lastSavedPoseValid = false;

ServoWrapper g_servos[SERVO_COUNT] = {
  ServoWrapper(SERVO_HEAD),
  ServoWrapper(SERVO_NECK),
  ServoWrapper(SERVO_HAND_LEFT),
  ServoWrapper(SERVO_HAND_RIGHT),
  ServoWrapper(SERVO_BODY)
};

uint16_t angleToPulse(float angle) {
  angle = constrain(angle, 0.0f, 180.0f);

  const float pulseUs =
    SERVO_MIN_US +
    (angle / 180.0f) *
    (SERVO_MAX_US - SERVO_MIN_US);

  const float counts =
    pulseUs * 4096.0f / 20000.0f + 0.5f;

  return (uint16_t)constrain(counts, 0.0f, 4095.0f);
}

float clampElectricalAngle(float angle) {
  return constrain(angle, 0.0f, 180.0f);
}

float clampServoAngle(int index, float angle) {
  if (index < 0 || index >= SERVO_COUNT) {
    return clampElectricalAngle(angle);
  }

  return constrain(
    angle,
    settingsServoMin(index),
    settingsServoMax(index)
  );
}

float clampAngle(int index, float angle, bool electrical) {
  if (electrical) {
    return clampElectricalAngle(angle);
  }

  return clampServoAngle(index, angle);
}

float servoRuntimeMid(int index) {
  return servoNormToDeg(index, 0.0f);
}

float servoNormToDeg(int index, float n) {
  if (index < 0 || index >= SERVO_COUNT) {
    return servoNormToDeg(n, 0.0f, 180.0f);
  }

  return servoNormToDeg(
    n,
    settingsServoMin(index),
    settingsServoMax(index)
  );
}

float servoDegToNorm(int index, float deg) {
  if (index < 0 || index >= SERVO_COUNT) {
    return servoDegToNorm(deg, 0.0f, 180.0f);
  }

  return servoDegToNorm(
    deg,
    settingsServoMin(index),
    settingsServoMax(index)
  );
}

void initServoOutputPin() {
  if (!PCA9685_OE_WIRED) {
    return;
  }

  pinMode(PCA9685_OE_PIN, OUTPUT);
  digitalWrite(PCA9685_OE_PIN, HIGH);
}

void disableServoOutputs() {
  if (!PCA9685_OE_WIRED) {
    return;
  }

  digitalWrite(PCA9685_OE_PIN, HIGH);
}

void enableServoOutputs() {
  if (!PCA9685_OE_WIRED) {
    return;
  }

  digitalWrite(PCA9685_OE_PIN, LOW);
}

namespace {

void fillDefaultPose(uint8_t out[SERVO_COUNT]) {
  for (int i = 0; i < SERVO_COUNT; i++) {
    out[i] = static_cast<uint8_t>(lroundf(servoRuntimeMid(i)));
  }
}

bool loadPoseFromNvs(uint8_t out[SERVO_COUNT]) {
  Preferences prefs;

  if (!prefs.begin(kPoseNs, true)) {
    return false;
  }

  uint8_t blob[SERVO_COUNT] = {};
  const size_t n = prefs.getBytes(kPoseKey, blob, SERVO_COUNT);
  prefs.end();

  if (n != static_cast<size_t>(SERVO_COUNT)) {
    return false;
  }

  for (int i = 0; i < SERVO_COUNT; i++) {
    out[i] = static_cast<uint8_t>(lroundf(clampServoAngle(i, blob[i])));
  }

  return true;
}

bool writePoseBlobToNvs(const uint8_t blob[SERVO_COUNT]) {
  Preferences prefs;

  if (!prefs.begin(kPoseNs, false)) {
    serialLogPrintln("[servo] pose NVS open failed");
    return false;
  }

  const size_t written = prefs.putBytes(kPoseKey, blob, SERVO_COUNT);
  prefs.end();

  if (written != static_cast<size_t>(SERVO_COUNT)) {
    serialLogPrintln("[servo] pose NVS write failed");
    return false;
  }

  memcpy(g_lastSavedPose, blob, SERVO_COUNT);
  g_lastSavedPoseValid = true;
  g_poseDirty = false;
  return true;
}

void markPoseDirty() {
  uint8_t blob[SERVO_COUNT];

  for (int i = 0; i < SERVO_COUNT; i++) {
    blob[i] = static_cast<uint8_t>(
      lroundf(clampServoAngle(i, g_servos[i].angle()))
    );
  }

  if (g_lastSavedPoseValid &&
      memcmp(blob, g_lastSavedPose, SERVO_COUNT) == 0) {
    g_poseDirty = false;
    return;
  }

  if (!g_poseDirty) {
    g_poseDirty = true;
    g_poseDirtySinceMs = millis();
  }
}

}  // namespace

bool flushServoPoseToNvs() {
  uint8_t blob[SERVO_COUNT];

  for (int i = 0; i < SERVO_COUNT; i++) {
    blob[i] = static_cast<uint8_t>(
      lroundf(clampServoAngle(i, g_servos[i].angle()))
    );
  }

  if (g_lastSavedPoseValid &&
      memcmp(blob, g_lastSavedPose, SERVO_COUNT) == 0) {
    g_poseDirty = false;
    return true;
  }

  return writePoseBlobToNvs(blob);
}

void pollServoPoseSave() {
  if (!g_poseDirty) {
    return;
  }

  if (millis() - g_poseDirtySinceMs < kPoseSaveDebounceMs) {
    return;
  }

  flushServoPoseToNvs();
}

bool servoOutputsHoldActive() {
  return g_parkHoldUntilReboot;
}

void setServoParkHoldUntilReboot(bool hold) {
  g_parkHoldUntilReboot = hold;
}

namespace {

// Skipped while the PCA9685 is unusable, so a dead bus does not stall the
// loop. The first failed write takes the chip down at once, with its outputs
// off, even inside a blocking move that never reaches the health check; the
// recovery rewrites every channel once the chip is back.
void writeChannel(uint8_t channel, uint16_t off) {
  if (!i2cServoDriverUp()) {
    return;
  }

  if (pwm.setPWM(channel, 0, off) != 0) {
    i2cNoteServoDriverFault("write failed");
  }
}

// Reads `count` registers from `reg` in one transfer; relies on auto-increment.
bool readPcaRegisters(uint8_t reg, uint8_t* values, uint8_t count) {
  Wire.beginTransmission(PCA9685_ADDRESS);
  Wire.write(reg);
  // Without a STOP nothing is sent yet; a failure shows up in requestFrom.
  Wire.endTransmission(false);

  if (Wire.requestFrom(PCA9685_ADDRESS, count) != count) {
    return false;
  }

  for (uint8_t i = 0; i < count; i++) {
    values[i] = static_cast<uint8_t>(Wire.read());
  }

  return true;
}

// Writes one channel and reads it back: an acknowledged write can still land
// corrupted on a bus that just glitched.
bool writeChannelVerified(uint8_t channel, uint16_t off) {
  if (pwm.setPWM(channel, 0, off) != 0) {
    return false;
  }

  uint8_t got[4] = {};
  const uint8_t want[4] = {0, 0, static_cast<uint8_t>(off & 0xFF), static_cast<uint8_t>(off >> 8)};

  return readPcaRegisters(PCA9685_LED0_ON_L + 4 * channel, got, 4) &&
         memcmp(got, want, sizeof(want)) == 0;
}

// Output mode and servo frequency, shared by boot and recovery. The chip must
// be awake first: setPWMFreq() restores the sleep bit it finds. False when the
// MODE2 write failed: the frequency is then skipped, since on a dead bus each
// of its transfers would wait out a timeout too.
bool writeServoSetup() {
  // The Adafruit driver never writes MODE2, so a value corrupted by a bus
  // glitch would outlive every restart. The chip keeps its registers across
  // an ESP32 reset: only its own power cycle clears them.
  Wire.beginTransmission(PCA9685_ADDRESS);
  Wire.write(kPcaMode2Reg);
  Wire.write(kPcaMode2Servo);

  if (Wire.endTransmission() != 0) {
    return false;
  }

  pwm.setPWMFreq(kServoPwmHz);
  return true;
}

// The end of boot: the recovery's sequence in one go. Outputs stay off unless
// the setup took: a wrong output mode or frequency must not reach the servos.
bool finishServoPwmDriverSetup() {
  ServoPwmDriverState state;

  if (!servoPwmDriverState(state) || !writeServoChannelsOff()) {
    return false;
  }

  delay(kServoOutputSettleMs);
  return enableServoOutputsWithChannels();
}

}  // namespace

bool servoPwmDriverState(ServoPwmDriverState& state) {
  state = {};

  // MODE1 and MODE2 are adjacent, so one read returns both. With
  // auto-increment off the second byte repeats MODE1 and fails the MODE2 check.
  uint8_t modes[2] = {};
  state.answered = readPcaRegisters(kPcaMode1Reg, modes, 2);

  if (!state.answered) {
    return false;
  }

  state.mode1 = modes[0];
  state.mode2 = modes[1];
  state.prescale = pwm.readPrescale();

  return (state.mode1 & kPcaMode1Mask) == kPcaMode1Servo &&
         state.mode2 == kPcaMode2Servo &&
         state.prescale == kPcaPrescaleServo;
}

bool setUpServoPwmDriver() {
  disableServoOutputs();

  // Not pwm.begin(): it programs 1 kHz before writeServoSetup() sets 50 Hz,
  // and on a board with OE not wired the live servos would get those pulses.
  // reset() wakes the chip, which is all begin() adds here.
  pwm.reset();

  if (!writeServoSetup()) {
    return false;
  }

  ServoPwmDriverState state;
  return servoPwmDriverState(state);
}

// Turning OE on is not synchronised to the PWM period: done while a channel is
// mid-pulse it would cut that pulse short, and the servo would read a wrong
// angle. Channel writes are: the PCA9685 applies new values at the end of a
// LOW cycle (datasheet 7.3.3). So the outputs come on with every channel off,
// and the channels are written after, each starting on a whole pulse.
bool writeServoChannelsOff() {
  disableServoOutputs();

  // Stops at the first failure: on a dead bus each further write waits out
  // its timeout, and the next attempt writes every channel anyway.
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (!writeChannelVerified(SERVO_SPECS[i].channel, kPca9685FullOff)) {
      return false;
    }
  }

  return true;
}

bool enableServoOutputsWithChannels() {
  enableServoOutputs();

  // The servos kept animating in software while the chip was unreachable;
  // this puts every channel where the firmware thinks it is.
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (!g_servos[i].resyncOutput()) {
      disableServoOutputs();
      return false;
    }
  }

  return true;
}

void initServoPwmDriver() {
  disableServoOutputs();

  pwm.begin();
  const bool setupWritten = writeServoSetup();
  delay(100);

  uint8_t pose[SERVO_COUNT];
  fillDefaultPose(pose);

  if (loadPoseFromNvs(pose)) {
    serialLogPrintln("[servo] restore last pose");
  } else {
    serialLogPrintln("[servo] no saved pose; using mid");
  }

  for (int i = 0; i < SERVO_COUNT; i++) {
    g_servos[i].restorePosition(static_cast<float>(pose[i]));
  }

  memcpy(g_lastSavedPose, pose, SERVO_COUNT);
  g_lastSavedPoseValid = true;
  g_poseDirty = false;

  // Same path as a restart, so boot and recovery cannot drift apart. A setup
  // that did not take leaves the outputs off and the recovery to the checks.
  if (!setupWritten || !finishServoPwmDriverSetup()) {
    i2cNoteServoDriverFault("setup failed");
  }
}

ServoWrapper& servoAt(int index) {
  return g_servos[index];
}

ServoWrapper::ServoWrapper(int index)
  : index_(index),
    angle_(servoMid(SERVO_SPECS[index])),
    target_(angle_),
    speedDegS_(SERVO_MAX_SPEED_DEG_S),
    lastUpdateMs_(0),
    lastPulse_(UINT16_MAX),
    released_(false) {}

float ServoWrapper::angle() const {
  return angle_;
}

void ServoWrapper::setTarget(float target, float speedDegS) {
  ensureServoOutput();
  target_ = clampServoAngle(index_, target);
  speedDegS_ = constrain(
    speedDegS,
    0.0f,
    SERVO_MAX_SPEED_DEG_S
  );
}

void ServoWrapper::setPosition(float angle) {
  angle = clampServoAngle(index_, angle);
  writeAngle(angle, false, false);
  target_ = angle_;
  lastUpdateMs_ = 0;
}

void ServoWrapper::restorePosition(float angle) {
  angle_ = clampServoAngle(index_, angle);
  target_ = angle_;
  lastUpdateMs_ = 0;
  // Not written yet: the next write for this servo must go out.
  lastPulse_ = UINT16_MAX;
}

void ServoWrapper::setNormTarget(float n, float speedDegS) {
  setTarget(servoNormToDeg(index_, n), speedDegS);
}

void ServoWrapper::setNormPosition(float n) {
  setPosition(servoNormToDeg(index_, n));
}

void ServoWrapper::stop() {
  target_ = angle_;
}

bool ServoWrapper::isMoving() const {
  return fabsf(target_ - angle_) >= SERVO_ANGLE_DEADBAND_DEG;
}

void ServoWrapper::update() {
  if (!isMoving()) {
    return;
  }

  const uint32_t now = millis();

  if (lastUpdateMs_ == 0) {
    lastUpdateMs_ = now;
    return;
  }

  const uint32_t elapsed = now - lastUpdateMs_;

  if (elapsed < (uint32_t)SERVO_STEP_MS) {
    return;
  }

  lastUpdateMs_ = now;

  const float maxStep =
    speedDegS_ * ((float)elapsed / 1000.0f);
  const float delta = target_ - angle_;
  const float step =
    constrain(delta, -maxStep, maxStep);
  const float next = angle_ + step;

  if (fabsf(target_ - next) < SERVO_ANGLE_DEADBAND_DEG) {
    writeAngle(target_, false, false);
    lastUpdateMs_ = 0;
    return;
  }

  writeAngle(next, false, false);
}

void updateAllServos() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    g_servos[i].update();
  }
}

bool anyServoMoving() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (g_servos[i].isMoving()) {
      return true;
    }
  }

  return false;
}

void ensureAllServoOutputs() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    g_servos[i].ensureServoOutput();
  }
}

void releaseAllServoOutputs() {
  if (g_allOutputsReleased) {
    return;
  }

  if (servoOutputsHoldActive()) {
    return;
  }

  flushServoPoseToNvs();

  for (int i = 0; i < SERVO_COUNT; i++) {
    g_servos[i].releaseServoOutput();
  }

  g_allOutputsReleased = true;
  serialLogPrintln("[servo] pwm off");
}

void ServoWrapper::ensureServoOutput() {
  if (!released_) {
    return;
  }

  released_ = false;
  g_allOutputsReleased = false;
  lastPulse_ = UINT16_MAX;

  const uint16_t pulse = angleToPulse(angle_);
  writeChannel(SERVO_SPECS[index_].channel, pulse);
  lastPulse_ = pulse;

  serialLogPrint("[servo] pwm on ");
  serialLogPrint(SERVO_SPECS[index_].name);
  serialLogPrint(" ch=");
  serialLogPrintln(SERVO_SPECS[index_].channel);
}

void ServoWrapper::releaseServoOutput() {
  if (released_) {
    return;
  }

  writeChannel(SERVO_SPECS[index_].channel, kPca9685FullOff);
  lastPulse_ = UINT16_MAX;
  released_ = true;
}

bool ServoWrapper::resyncOutput() {
  // Written straight to the chip, past writeChannel()'s check: this runs at
  // boot and while the chip still counts as down.
  if (released_) {
    lastPulse_ = UINT16_MAX;
    return writeChannelVerified(SERVO_SPECS[index_].channel, kPca9685FullOff);
  }

  const uint16_t pulse = angleToPulse(angle_);

  if (!writeChannelVerified(SERVO_SPECS[index_].channel, pulse)) {
    // Not cached: the next write for this servo must go out.
    lastPulse_ = UINT16_MAX;
    return false;
  }

  lastPulse_ = pulse;
  return true;
}

void ServoWrapper::writeAngle(float angle, bool log, bool electrical) {
  ensureServoOutput();
  angle = clampAngle(index_, angle, electrical);

  const uint16_t pulse = angleToPulse(angle);

  if (log) {
    serialLogPrint("Servo ");
    serialLogPrint(SERVO_SPECS[index_].name);
    serialLogPrint(" -> ");
    serialLogPrint(angle);
    serialLogPrint(" deg, pulse: ");
    serialLogPrintln(pulse);
  }

  if (pulse != lastPulse_) {
    writeChannel(SERVO_SPECS[index_].channel, pulse);
    lastPulse_ = pulse;
  }

  angle_ = angle;
  markPoseDirty();
}

void ServoWrapper::snapTo(float angle) {
  writeAngle(angle, true, false);
  target_ = angle_;
  lastUpdateMs_ = 0;
}

bool ServoWrapper::moveTo(float target) {
  target = clampServoAngle(index_, target);
  stop();

  const float fromAngle = angle_;
  const float delta = fabsf(target - fromAngle);

  if (delta < SERVO_ANGLE_DEADBAND_DEG) {
    writeAngle(target, true, false);
    target_ = angle_;
    lastUpdateMs_ = 0;
    return true;
  }

  const int durationMs =
    (int)((delta / SERVO_MAX_SPEED_DEG_S) * 1000.0f);

  const int minSteps =
    (int)(delta / SERVO_PWM_STEP_DEG) + 1;
  const int steps = max(
    max(1, durationMs / SERVO_STEP_MS),
    minSteps
  );
  const int stepDelayMs = max(1, durationMs / steps);

  serialLogPrint("Servo ");
  serialLogPrint(SERVO_SPECS[index_].name);
  serialLogPrint(" smooth ");
  serialLogPrint(fromAngle);
  serialLogPrint(" -> ");
  serialLogPrint(target);
  serialLogPrint(" deg (");
  serialLogPrint(SERVO_MAX_SPEED_DEG_S);
  serialLogPrintln(" deg/s)");

  for (int step = 1; step <= steps; step++) {
    const float progress =
      anim::easeInOutCubic((float)step / (float)steps);

    const float angle =
      fromAngle +
      (target - fromAngle) *
      progress;

    writeAngle(angle, false, false);
    delay(stepDelayMs);
  }

  writeAngle(target, true, false);
  target_ = angle_;
  lastUpdateMs_ = 0;
  return true;
}

bool ServoWrapper::moveToElectrical(float target, float speedDegS) {
  target = clampElectricalAngle(target);
  speedDegS = constrain(speedDegS, 1.0f, SERVO_MAX_SPEED_DEG_S);
  stop();

  const float fromAngle = angle_;
  const float delta = fabsf(target - fromAngle);

  if (delta < SERVO_ANGLE_DEADBAND_DEG) {
    writeAngle(target, true, true);
    target_ = angle_;
    lastUpdateMs_ = 0;
    return true;
  }

  const int durationMs =
    (int)((delta / speedDegS) * 1000.0f);

  const int minSteps =
    (int)(delta / SERVO_PWM_STEP_DEG) + 1;
  const int steps = max(
    max(1, durationMs / SERVO_STEP_MS),
    minSteps
  );
  const int stepDelayMs = max(1, durationMs / steps);

  serialLogPrint("Servo ");
  serialLogPrint(SERVO_SPECS[index_].name);
  serialLogPrint(" calib ");
  serialLogPrint(fromAngle);
  serialLogPrint(" -> ");
  serialLogPrint(target);
  serialLogPrint(" deg (");
  serialLogPrint(speedDegS);
  serialLogPrintln(" deg/s)");

  for (int step = 1; step <= steps; step++) {
    const float progress =
      anim::easeInOutCubic((float)step / (float)steps);

    const float angle =
      fromAngle +
      (target - fromAngle) *
      progress;

    writeAngle(angle, false, true);
    delay(stepDelayMs);
  }

  writeAngle(target, true, true);
  target_ = angle_;
  lastUpdateMs_ = 0;
  return true;
}

void servoMoveAllSmoothTo(
  const float targets[SERVO_COUNT],
  float speedDegS
) {
  float fromAngles[SERVO_COUNT];
  float clampedTargets[SERVO_COUNT];
  float maxDelta = 0.0f;

  speedDegS = constrain(
    speedDegS,
    0.0f,
    SERVO_MAX_SPEED_DEG_S
  );

  for (int i = 0; i < SERVO_COUNT; i++) {
    fromAngles[i] = g_servos[i].angle_;
    clampedTargets[i] = clampServoAngle(i, targets[i]);
    maxDelta = max(
      maxDelta,
      fabsf(clampedTargets[i] - fromAngles[i])
    );
  }

  if (maxDelta < SERVO_ANGLE_DEADBAND_DEG) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      g_servos[i].writeAngle(clampedTargets[i], true, false);
      g_servos[i].target_ = g_servos[i].angle_;
      g_servos[i].lastUpdateMs_ = 0;
    }
    return;
  }

  const int durationMs =
    (int)((maxDelta / speedDegS) * 1000.0f);

  const int minSteps =
    (int)(maxDelta / SERVO_PWM_STEP_DEG) + 1;
  const int steps = max(
    max(1, durationMs / SERVO_STEP_MS),
    minSteps
  );
  const int stepDelayMs = max(1, durationMs / steps);

  serialLogPrint("All servos smooth (");
  serialLogPrint(speedDegS);
  serialLogPrintln(" deg/s)");

  for (int step = 1; step <= steps; step++) {
    const float progress =
      anim::easeInOutCubic((float)step / (float)steps);

    for (int i = 0; i < SERVO_COUNT; i++) {
      const float angle =
        fromAngles[i] +
        (clampedTargets[i] - fromAngles[i]) *
        progress;

      g_servos[i].writeAngle(angle, false, false);
    }

    delay(stepDelayMs);
  }

  for (int i = 0; i < SERVO_COUNT; i++) {
    g_servos[i].writeAngle(clampedTargets[i], true, false);
    g_servos[i].target_ = g_servos[i].angle_;
    g_servos[i].lastUpdateMs_ = 0;
  }
}

void servoMoveAllToElectrical(
  const float targets[SERVO_COUNT],
  float speedDegS
) {
  float fromAngles[SERVO_COUNT];
  float clampedTargets[SERVO_COUNT];
  float maxDelta = 0.0f;

  speedDegS = constrain(
    speedDegS,
    1.0f,
    SERVO_MAX_SPEED_DEG_S
  );

  for (int i = 0; i < SERVO_COUNT; i++) {
    fromAngles[i] = g_servos[i].angle_;
    clampedTargets[i] = clampElectricalAngle(targets[i]);
    maxDelta = max(
      maxDelta,
      fabsf(clampedTargets[i] - fromAngles[i])
    );
  }

  if (maxDelta < SERVO_ANGLE_DEADBAND_DEG) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      g_servos[i].writeAngle(clampedTargets[i], true, true);
      g_servos[i].target_ = g_servos[i].angle_;
      g_servos[i].lastUpdateMs_ = 0;
    }
    return;
  }

  const int durationMs =
    (int)((maxDelta / speedDegS) * 1000.0f);

  const int minSteps =
    (int)(maxDelta / SERVO_PWM_STEP_DEG) + 1;
  const int steps = max(
    max(1, durationMs / SERVO_STEP_MS),
    minSteps
  );
  const int stepDelayMs = max(1, durationMs / steps);

  serialLogPrint("All servos calib (");
  serialLogPrint(speedDegS);
  serialLogPrintln(" deg/s)");

  for (int step = 1; step <= steps; step++) {
    const float progress =
      anim::easeInOutCubic((float)step / (float)steps);

    for (int i = 0; i < SERVO_COUNT; i++) {
      const float angle =
        fromAngles[i] +
        (clampedTargets[i] - fromAngles[i]) *
        progress;

      g_servos[i].writeAngle(angle, false, true);
    }

    delay(stepDelayMs);
  }

  for (int i = 0; i < SERVO_COUNT; i++) {
    g_servos[i].writeAngle(clampedTargets[i], true, true);
    g_servos[i].target_ = g_servos[i].angle_;
    g_servos[i].lastUpdateMs_ = 0;
  }
}
