#include "hardware/i2c_fault.h"

I2cFaultAction I2cFaultTracker::due(uint32_t now) const {
  // Unsigned subtraction keeps the intervals right across the millis() wrap.
  if (down_) {
    return now - lastAttemptMs_ >= retryDelayMs_
      ? I2cFaultAction::Recover
      : I2cFaultAction::None;
  }

  return !checked_ || now - lastCheckMs_ >= kI2cCheckIntervalMs
    ? I2cFaultAction::Check
    : I2cFaultAction::None;
}

bool I2cFaultTracker::noteCheck(bool ok, uint32_t now) {
  checked_ = true;
  lastCheckMs_ = now;

  if (ok) {
    failures_ = 0;
    return false;
  }

  if (++failures_ < kI2cFailuresToFault) {
    return false;
  }

  down_ = true;
  faults_++;
  lastFaultMs_ = now;
  attempts_ = 0;
  // The first recovery attempt runs right away.
  retryDelayMs_ = 0;
  lastAttemptMs_ = now;
  return true;
}

void I2cFaultTracker::noteRecovery(bool ok, uint32_t now) {
  attempts_++;
  lastAttemptMs_ = now;

  if (ok) {
    down_ = false;
    failures_ = 0;
    recoveries_++;
    lastCheckMs_ = now;
    return;
  }

  if (retryDelayMs_ == 0) {
    retryDelayMs_ = kI2cFirstRetryMs;
  } else if (retryDelayMs_ >= kI2cMaxRetryMs / 2) {
    retryDelayMs_ = kI2cMaxRetryMs;
  } else {
    retryDelayMs_ *= 2;
  }
}
