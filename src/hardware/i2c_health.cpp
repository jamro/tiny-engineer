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
  bool (*recover)();
  // Extra text for the fault log line; null when there is none.
  void (*logFaultDetail)();
  I2cFaultTracker tracker;
};

bool servoDriverHealthy();
void stopServoWrites();
bool recoverServoDriver();
void logServoDriverRegisters();
bool oledHealthy();
void stopOledWrites();
bool recoverOled();

Device g_servoDriver = {
  "pca9685", servoDriverHealthy, stopServoWrites, recoverServoDriver, logServoDriverRegisters, {}
};
Device g_oled = {"oled", oledHealthy, stopOledWrites, recoverOled, nullptr, {}};

bool g_started = false;
// The robot runs without an OLED; only watch one that was there at boot.
bool g_oledExpected = false;
bool g_servoDriverUp = true;
// False after a failed Wire restart. Wire must not be used then: without its
// buffers a transfer leaks Wire's lock, and the next restart would wait on
// that lock forever.
bool g_wireUp = true;
// False while the last bus restart found a line held low.
bool g_linesFree = true;
// Registers from the latest PCA9685 check, for the fault log line.
ServoPwmDriverState g_servoDriverState = {};

// A device interrupted mid-byte keeps SDA low until it has been clocked
// through the rest of that byte. Clock it out, send a STOP, and report where
// the lines settled: a line still low means something is holding the bus.
BusLines clearBus() {
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, OUTPUT_OPEN_DRAIN);
  digitalWrite(I2C_SCL, HIGH);
  delayMicroseconds(kBusClearHalfPeriodUs);

  for (uint8_t i = 0; i < kBusClearPulses && digitalRead(I2C_SDA) == LOW; i++) {
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

void logFault(const Device& device, uint32_t now) {
  serialLogPrint("[i2c] ");
  serialLogPrint(device.name);
  serialLogPrint(" FAULT #");
  serialLogPrint(device.tracker.faults());
  serialLogPrint(" at ");
  serialLogPrint(now);
  serialLogPrint(" ms");

  if (device.logFaultDetail != nullptr) {
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
// line is still held: every transfer would fail, so callers skip the probes
// and their timeouts.
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
  g_linesFree = lines.sdaHigh && lines.sclHigh;
  g_wireUp = Wire.begin(I2C_SDA, I2C_SCL);

  if (!g_wireUp) {
    stopOledWrites();
  }

  serialLogPrint(g_wireUp ? "[i2c] bus restarted" : "[i2c] Wire.begin failed");
  logLines(lines);

  return g_wireUp && g_linesFree;
}

// The restart leaves the panel lit; a sleeping robot keeps it dark. Awake,
// the next eye frame or setup screen redraws it.
bool restartOledForSleepState() {
  if (!restartOled()) {
    return false;
  }

  if (isSleeping()) {
    blankOled();
    sleepOled();
  }

  return true;
}

// Both chips share a supply, so a PCA9685 that had to be set up again is
// reason to set the panel up again too, even when it still answers.
void restartOledAfterBusRecovery() {
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
  return g_wireUp && servoPwmDriverState(g_servoDriverState);
}

void stopServoWrites() {
  g_servoDriverUp = false;
}

// A PCA9685 that answered but lost its setup: the registers say what changed.
void logServoDriverRegisters() {
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

// The PCA9685 is the required chip, so its recovery is the one that restarts
// the bus. A chip that still answers proves the bus works: then only its
// setup is redone.
bool recoverServoDriver() {
  bool reachable = servoDriverAnswers();

  if (!reachable) {
    reachable = restartBus() && servoDriverAnswers();
  }

  if (!reachable) {
    return false;
  }

  // An answer proves the lines are free, whatever the last restart saw.
  g_linesFree = true;

  g_servoDriverUp = restartServoPwmDriver();

  if (g_servoDriverUp) {
    restartOledAfterBusRecovery();
  }

  return g_servoDriverUp;
}

bool oledHealthy() {
  return g_wireUp && i2cDeviceConnected(OLED_ADDRESS);
}

void stopOledWrites() {
  oledAvailable = false;
}

// Never restarts the bus: an OLED fault alone must not disturb the servos.
// A dead bus is left to the PCA9685's recovery.
bool recoverOled() {
  return g_wireUp && g_linesFree && restartOledForSleepState();
}

void service(Device& device, uint32_t now) {
  switch (device.tracker.due(now)) {
    case I2cFaultAction::Check:
      if (device.tracker.noteCheck(device.healthy(), now)) {
        // Each failed write blocks the loop for its timeout.
        device.stopWrites();
        logFault(device, now);
      }
      break;
    case I2cFaultAction::Recover:
      noteRecovery(device, device.recover());
      break;
    case I2cFaultAction::None:
      break;
  }
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

  g_wireUp = Wire.begin(I2C_SDA, I2C_SCL);
}

void beginI2cHealth() {
  g_oledExpected = oledAvailable;
  g_started = true;
}

void updateI2cHealth(uint32_t now) {
  if (!g_started) {
    return;
  }

  service(g_servoDriver, now);

  if (g_oledExpected) {
    service(g_oled, now);
  }
}

bool i2cServoDriverUp() {
  return g_servoDriverUp;
}

I2cHealth i2cHealth() {
  return {
    !g_servoDriver.tracker.down() && !g_oled.tracker.down(),
    deviceHealth(g_servoDriver, g_servoDriverUp),
    deviceHealth(g_oled, oledAvailable)
  };
}
