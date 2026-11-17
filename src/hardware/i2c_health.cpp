#include <Arduino.h>
#include <Wire.h>

#include "display/oled.h"
#include "hardware/i2c_fault.h"
#include "hardware/i2c_health.h"
#include "hardware/servo_wrapper.h"
#include "pins.h"
#include "serial_log.h"
#include "sleep.h"

namespace {

constexpr uint8_t kBusClearPulses = 9;
constexpr uint32_t kBusClearHalfPeriodUs = 5;
// Wire's default is 50 ms. Every transfer here finishes in a few: the
// largest, a 128-byte OLED frame chunk, takes about 3 ms at 400 kHz and 12 ms
// at 100 kHz. On a dead bus each transfer waits this long, and a tick does at
// most one device's work, about one timeout. The speaker's DMA buffer holds
// about 65 ms (6 x 240 frames at 22,050 Hz), so a tick never starves it.
constexpr uint16_t kI2cTransferTimeoutMs = 20;

enum class RecoveryResult : uint8_t {
  InProgress,  // a step ran; call again next tick
  Succeeded,
  Failed
};

struct BusLines {
  bool sdaHigh;
  bool sclHigh;
};

// One watched chip. Each has its own tracker, so a chip that stays down does
// not stop the checks on the other, and a fault on one does not restart both.
struct Device {
  const char* name;
  bool (*healthy)();
  void (*stopWrites)();
  // One step of a recovery, bounded by about one transfer timeout.
  RecoveryResult (*recover)(uint32_t now);
  // Extra text for the fault log line; null when there is none.
  void (*logFaultDetail)();
  I2cFaultTracker tracker;
};

bool servoDriverHealthy();
RecoveryResult recoverServoDriver(uint32_t now);
void logServoDriverDetail();
bool oledHealthy();
void stopOledWrites();
RecoveryResult recoverOled(uint32_t now);

Device g_servoDriver = {
  // At a fault the outputs go off, so a corrupted setup cannot keep driving
  // the servos. Recovery turns them back on once the setup reads back right.
  "pca9685", servoDriverHealthy, disableServoOutputs, recoverServoDriver, logServoDriverDetail, {}
};
Device g_oled = {"oled", oledHealthy, stopOledWrites, recoverOled, nullptr, {}};

bool g_started = false;
// The robot runs without an OLED; only watch one that was there at boot,
// including one whose setup failed there, so that setup gets retried.
bool g_oledExpected = false;
// False after a failed Wire restart. Wire must not be used then: without its
// buffers a transfer leaks Wire's lock, and the next restart would wait on
// that lock forever.
bool g_wireUp = true;
// Set when the PCA9685 is found without its setup; cleared once the OLED has
// been set up again. Kept across failed attempts: a retry finds the PCA9685's
// setup restored and would otherwise skip the panel.
bool g_oledRestartPending = false;
// Where the PCA9685's recovery is. It runs one step per tick, each a short
// burst of transfers, so a recovery never holds the loop for long.
enum class ServoRecoveryStep : uint8_t {
  Probe,        // does the chip answer?
  RestartBus,   // it did not: restart Wire, then ask again
  CheckSetup,   // does it still hold its setup?
  SetUp,        // it did not: set it up again
  ChannelsOff,  // every channel off, outputs off
  Settle,       // wait for the off values to take effect
  Oled,         // set the panel up again after a power loss
  ChannelsOn    // outputs on, then every channel written; ends the recovery
};
ServoRecoveryStep g_servoStep = ServoRecoveryStep::Probe;
uint32_t g_channelsOffMs = 0;
// Registers from the latest PCA9685 check, for the fault log line.
ServoPwmDriverState g_servoDriverState = {};

// A device interrupted mid-byte keeps driving SDA until it has been clocked
// through the rest of that byte. Clock it out, send a STOP, and report where
// the lines settled: a line still low means something is holding the bus.
// All nine clocks, even once SDA reads high: in an interrupted read a high
// SDA can be a 1 data bit, and the device would pull the line low again under
// the STOP. Nine clocks always pass an acknowledge slot, where the released
// SDA reads as a NACK and ends the read.
BusLines clearBus() {
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, OUTPUT_OPEN_DRAIN);
  digitalWrite(I2C_SCL, HIGH);
  delayMicroseconds(kBusClearHalfPeriodUs);

  for (uint8_t i = 0; i < kBusClearPulses; i++) {
    digitalWrite(I2C_SCL, LOW);
    delayMicroseconds(kBusClearHalfPeriodUs);
    digitalWrite(I2C_SCL, HIGH);
    delayMicroseconds(kBusClearHalfPeriodUs);
  }

  // STOP: SDA rises while SCL is high.
  digitalWrite(I2C_SCL, LOW);
  delayMicroseconds(kBusClearHalfPeriodUs);
  pinMode(I2C_SDA, OUTPUT_OPEN_DRAIN);
  digitalWrite(I2C_SDA, LOW);
  delayMicroseconds(kBusClearHalfPeriodUs);
  digitalWrite(I2C_SCL, HIGH);
  delayMicroseconds(kBusClearHalfPeriodUs);
  digitalWrite(I2C_SDA, HIGH);
  delayMicroseconds(kBusClearHalfPeriodUs);

  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delayMicroseconds(kBusClearHalfPeriodUs);

  return {digitalRead(I2C_SDA) == HIGH, digitalRead(I2C_SCL) == HIGH};
}

void logLines(const BusLines& lines) {
  serialLogPrint(" sda=");
  serialLogPrint(lines.sdaHigh ? "high" : "LOW");
  serialLogPrint(" scl=");
  serialLogPrintln(lines.sclHigh ? "high" : "LOW");
}

// `cause` names a fault found outside a check; a check's fault logs the
// device's own detail instead.
void logFault(const Device& device, uint32_t now, const char* cause = nullptr) {
  serialLogPrint("[i2c] ");
  serialLogPrint(device.name);
  serialLogPrint(" FAULT #");
  serialLogPrint(device.tracker.faults());
  serialLogPrint(" at ");
  serialLogPrint(now);
  serialLogPrint(" ms");

  if (cause != nullptr) {
    serialLogPrint(" ");
    serialLogPrint(cause);
  } else if (device.logFaultDetail != nullptr) {
    device.logFaultDetail();
  }

  serialLogPrintln();
}

void noteRecovery(Device& device, bool ok) {
  const uint32_t now = millis();
  device.tracker.noteRecovery(ok, now);

  serialLogPrint("[i2c] ");
  serialLogPrint(device.name);

  if (ok) {
    serialLogPrint(" recovered, attempt ");
    serialLogPrint(device.tracker.attempts());
    serialLogPrint(", down ");
    serialLogPrint(now - device.tracker.lastFaultMs());
    serialLogPrintln(" ms");
    return;
  }

  serialLogPrint(" still down, attempt ");
  serialLogPrint(device.tracker.attempts());
  serialLogPrint(", retry in ");
  serialLogPrint(device.tracker.retryDelayMs());
  serialLogPrintln(" ms");
}

// Restarts Wire with the lines freed. False when Wire did not come back or a
// line is still held: every transfer would fail, so the caller skips the
// probes and their timeouts. Only the PCA9685's recovery calls this, so servo
// writes are already off.
bool restartBus() {
  g_wireUp = false;

  if (!Wire.end()) {
    // Wire dropped its buffers but kept its driver. begin() would report
    // success without them, so leave it down and end() again next attempt.
    stopOledWrites();
    serialLogPrintln("[i2c] Wire.end failed");
    return false;
  }

  const BusLines lines = clearBus();
  g_wireUp = Wire.begin(I2C_SDA, I2C_SCL);
  const bool usable = g_wireUp && lines.sdaHigh && lines.sclHigh;

  // A held line fails every transfer as surely as a dead Wire: stop the
  // OLED's frames, and let its own checks recover it once the bus is back.
  if (!usable) {
    stopOledWrites();
  }

  serialLogPrint(g_wireUp ? "[i2c] bus restarted" : "[i2c] Wire.begin failed");
  logLines(lines);

  return usable;
}

// The restart puts the last frame back on the panel; a sleeping robot keeps
// it dark.
bool restartOledForSleepState() {
  return restartOled(!isSleeping());
}

// Both chips share a supply, so a PCA9685 that lost its setup is reason to set
// the panel up again too, even when it still answers. A failure here while the
// OLED counts as up clears oledAvailable, and the OLED's own checks then take
// it down and recover it.
void restartOledAfterPowerLoss() {
  g_oledRestartPending = false;

  if (!g_oledExpected) {
    return;
  }

  const bool ok = restartOledForSleepState();

  if (g_oled.tracker.down()) {
    noteRecovery(g_oled, ok);
  }
}

bool servoDriverHealthy() {
  g_servoDriverState = {};

  if (!g_wireUp) {
    return false;
  }

  return servoPwmDriverState(g_servoDriverState);
}

// Why a check faulted the PCA9685: the registers it answered with.
void logServoDriverDetail() {
  if (!g_servoDriverState.answered) {
    return;
  }

  serialLogPrint(" mode1=0x");
  serialLogPrint(g_servoDriverState.mode1, HEX);
  serialLogPrint(" mode2=0x");
  serialLogPrint(g_servoDriverState.mode2, HEX);
  serialLogPrint(" prescale=0x");
  serialLogPrint(g_servoDriverState.prescale, HEX);
}

bool servoDriverAnswers() {
  return g_wireUp && i2cDeviceConnected(PCA9685_ADDRESS);
}

RecoveryResult endServoRecovery(RecoveryResult result) {
  g_servoStep = ServoRecoveryStep::Probe;
  return result;
}

RecoveryResult nextServoStep(ServoRecoveryStep step) {
  g_servoStep = step;
  return RecoveryResult::InProgress;
}

// The PCA9685 is the required chip, so its recovery is the one that restarts
// the bus. A chip that still answers proves the bus works. One that also kept
// its setup only lost a write or a read: rewriting the channels is enough, and
// the healthy OLED is left alone. A lost setup means the chip lost power or
// was corrupted: it is set up from scratch, and so is the panel.
RecoveryResult recoverServoDriver(uint32_t now) {
  switch (g_servoStep) {
    case ServoRecoveryStep::Probe:
      return nextServoStep(
        servoDriverAnswers() ? ServoRecoveryStep::CheckSetup : ServoRecoveryStep::RestartBus
      );

    case ServoRecoveryStep::RestartBus:
      if (!restartBus() || !servoDriverAnswers()) {
        return endServoRecovery(RecoveryResult::Failed);
      }
      return nextServoStep(ServoRecoveryStep::CheckSetup);

    case ServoRecoveryStep::CheckSetup:
      if (servoPwmDriverState(g_servoDriverState)) {
        return nextServoStep(ServoRecoveryStep::ChannelsOff);
      }
      g_oledRestartPending = true;
      return nextServoStep(ServoRecoveryStep::SetUp);

    case ServoRecoveryStep::SetUp:
      if (!setUpServoPwmDriver()) {
        return endServoRecovery(RecoveryResult::Failed);
      }
      return nextServoStep(ServoRecoveryStep::ChannelsOff);

    case ServoRecoveryStep::ChannelsOff:
      if (!writeServoChannelsOff()) {
        return endServoRecovery(RecoveryResult::Failed);
      }
      // After the writes, not `now`: they must not eat into the settle time.
      g_channelsOffMs = millis();
      return nextServoStep(ServoRecoveryStep::Settle);

    case ServoRecoveryStep::Settle:
      if (now - g_channelsOffMs < kServoOutputSettleMs) {
        return RecoveryResult::InProgress;
      }
      return nextServoStep(
        g_oledRestartPending ? ServoRecoveryStep::Oled : ServoRecoveryStep::ChannelsOn
      );

    case ServoRecoveryStep::Oled:
      restartOledAfterPowerLoss();
      return nextServoStep(ServoRecoveryStep::ChannelsOn);

    // Last, so the channels written here are the latest the firmware asked
    // for: the chip counts as up again in the same pass. A step after it
    // would leave a pass where servo writes are still skipped but cached.
    case ServoRecoveryStep::ChannelsOn:
      if (!enableServoOutputsWithChannels()) {
        return endServoRecovery(RecoveryResult::Failed);
      }
      return endServoRecovery(RecoveryResult::Succeeded);
  }

  return endServoRecovery(RecoveryResult::Failed);
}

// oledAvailable is part of the check: a restart that failed outside an OLED
// fault cleared it, and only a fault and its recovery set it again.
bool oledHealthy() {
  return g_wireUp && oledAvailable && i2cDeviceConnected(OLED_ADDRESS);
}

void stopOledWrites() {
  oledAvailable = false;
}

// Never restarts the bus: an OLED fault alone must not disturb the servos.
// A dead bus is left to the PCA9685's recovery.
RecoveryResult recoverOled(uint32_t) {
  return g_wireUp && restartOledForSleepState()
    ? RecoveryResult::Succeeded
    : RecoveryResult::Failed;
}

// The device is down: stop its writes, since each failed one blocks the loop
// for its timeout.
void enterFault(Device& device, uint32_t now, const char* cause = nullptr) {
  device.stopWrites();
  logFault(device, now, cause);
}

// True when it did bus work: a check, or a step of a recovery.
bool service(Device& device) {
  // Read here, not once per tick: the other chip's work can stamp this
  // tracker with a later time.
  const uint32_t now = millis();

  switch (device.tracker.due(now)) {
    case I2cFaultAction::Check:
      if (device.tracker.noteCheck(device.healthy(), now)) {
        enterFault(device, now);
      }
      return true;
    case I2cFaultAction::Recover: {
      // A recovery in progress keeps its tracker due until it ends.
      const RecoveryResult result = device.recover(now);

      if (result != RecoveryResult::InProgress) {
        noteRecovery(device, result == RecoveryResult::Succeeded);
      }
      return true;
    }
    case I2cFaultAction::None:
      return false;
  }

  return false;
}

I2cDeviceHealth deviceHealth(const Device& device, bool up) {
  return {
    up,
    device.tracker.faults(),
    device.tracker.recoveries(),
    device.tracker.lastFaultMs()
  };
}

}  // namespace

void initI2cBus() {
  const BusLines lines = clearBus();

  if (!lines.sdaHigh || !lines.sclHigh) {
    serialLogPrint("[i2c] bus held at boot:");
    logLines(lines);
  }

  // Kept across Wire restarts: it is a member, set once.
  Wire.setTimeOut(kI2cTransferTimeoutMs);
  g_wireUp = Wire.begin(I2C_SDA, I2C_SCL);

  if (!g_wireUp) {
    serialLogPrintln("[i2c] Wire.begin failed at boot");
  }
}

void beginI2cHealth() {
  g_oledExpected = oledDetectedAtBoot();
  g_started = true;
}

void updateI2cHealth() {
  if (!g_started) {
    return;
  }

  // At most one chip's work per tick, so a tick costs about one transfer
  // timeout at worst. The OLED's turn comes a tick later.
  if (!service(g_servoDriver) && g_oledExpected) {
    service(g_oled);
  }
}

bool i2cServoDriverUp() {
  return !g_servoDriver.tracker.down();
}

void i2cNoteServoDriverFault(const char* cause) {
  const uint32_t now = millis();

  if (g_servoDriver.tracker.noteFault(now)) {
    enterFault(g_servoDriver, now, cause);
  }
}

I2cHealth i2cHealth() {
  const bool servoDriverUp = i2cServoDriverUp();

  return {
    servoDriverUp && (!g_oledExpected || oledAvailable),
    deviceHealth(g_servoDriver, servoDriverUp),
    deviceHealth(g_oled, oledAvailable)
  };
}
