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

I2cFaultTracker g_tracker;
bool g_started = false;
// The robot runs without an OLED; only watch one that was there at boot.
bool g_oledExpected = false;
bool g_servoDriverUp = true;

struct BusLines {
  bool sdaHigh;
  bool sclHigh;
};

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

void logDevices(bool servoDriver, bool oled) {
  serialLogPrint(" pca9685=");
  serialLogPrint(servoDriver ? "ok" : "fail");
  serialLogPrint(" oled=");
  serialLogPrint(!g_oledExpected ? "absent" : oled ? "ok" : "fail");
}

// Restarts whatever is down. Returns true when every expected device is up.
bool bringUp() {
  bool busUsable = true;

  // A PCA9685 that still answers proves the bus works: leave it alone and
  // only retry the OLED.
  if (!(g_servoDriverUp && servoPwmDriverResponds())) {
    g_servoDriverUp = false;
    oledAvailable = false;

    Wire.end();
    const BusLines lines = clearBus();
    Wire.begin(I2C_SDA, I2C_SCL);

    serialLogPrint("[i2c] bus restarted");
    logLines(lines);

    // A held line fails every transfer; skip the probes and their timeouts.
    busUsable = lines.sdaHigh && lines.sclHigh;

    if (busUsable && i2cDeviceConnected(PCA9685_ADDRESS)) {
      restartServoPwmDriver();
      g_servoDriverUp = servoPwmDriverResponds();
    }
  }

  if (busUsable && g_oledExpected && !oledAvailable && restartOled()) {
    // The restart turned the panel on; a sleeping robot keeps it dark. Awake,
    // the next eye frame or setup screen redraws it.
    if (isSleeping()) {
      blankOled();
      sleepOled();
    }
  }

  return g_servoDriverUp && (!g_oledExpected || oledAvailable);
}

void check(uint32_t now) {
  ServoPwmDriverState pca;
  const bool servoDriver = servoPwmDriverState(pca);
  const bool oled = !g_oledExpected || i2cDeviceConnected(OLED_ADDRESS);

  if (!g_tracker.noteCheck(servoDriver && oled, now)) {
    return;
  }

  // Stop writing to both: they share the bus, and each failed write blocks
  // the loop for its timeout.
  g_servoDriverUp = false;
  oledAvailable = false;

  serialLogPrint("[i2c] FAULT #");
  serialLogPrint(g_tracker.faults());
  serialLogPrint(" at ");
  serialLogPrint(now);
  serialLogPrint(" ms:");
  logDevices(servoDriver, oled);

  // It answered but lost its setup: the registers say what changed.
  if (!servoDriver && pca.answered) {
    serialLogPrint(" mode1=0x");
    serialLogPrint(pca.mode1, HEX);
    serialLogPrint(" mode2=0x");
    serialLogPrint(pca.mode2, HEX);
    serialLogPrint(" prescale=0x");
    serialLogPrint(pca.prescale, HEX);
  }

  serialLogPrintln();
}

void recover() {
  const bool ok = bringUp();
  g_tracker.noteRecovery(ok, millis());

  if (ok) {
    serialLogPrint("[i2c] recovered, attempt ");
    serialLogPrint(g_tracker.attempts());
    serialLogPrint(", down ");
    serialLogPrint(millis() - g_tracker.lastFaultMs());
    serialLogPrintln(" ms");
    return;
  }

  serialLogPrint("[i2c] still down, attempt ");
  serialLogPrint(g_tracker.attempts());
  serialLogPrint(":");
  logDevices(g_servoDriverUp, oledAvailable);
  serialLogPrint(" retry in ");
  serialLogPrint(g_tracker.retryDelayMs());
  serialLogPrintln(" ms");
}

}  // namespace

void initI2cBus() {
  const BusLines lines = clearBus();

  if (!lines.sdaHigh || !lines.sclHigh) {
    serialLogPrint("[i2c] bus held at boot:");
    logLines(lines);
  }

  Wire.begin(I2C_SDA, I2C_SCL);
}

void beginI2cHealth() {
  g_oledExpected = oledAvailable;
  g_started = true;
}

void updateI2cHealth(uint32_t now) {
  if (!g_started) {
    return;
  }

  switch (g_tracker.due(now)) {
    case I2cFaultAction::Check:
      check(now);
      break;
    case I2cFaultAction::Recover:
      recover();
      break;
    case I2cFaultAction::None:
      break;
  }
}

bool i2cServoDriverUp() {
  return g_servoDriverUp;
}

I2cHealth i2cHealth() {
  return {
    !g_tracker.down(),
    g_servoDriverUp,
    g_tracker.faults(),
    g_tracker.recoveries(),
    g_tracker.lastFaultMs()
  };
}
