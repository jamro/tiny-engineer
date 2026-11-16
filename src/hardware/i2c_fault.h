#pragma once

#include <cstdint>

constexpr uint32_t kI2cCheckIntervalMs = 500;
// One failed check can be a glitch; two in a row take the bus down.
constexpr uint8_t kI2cFailuresToFault = 2;
constexpr uint32_t kI2cFirstRetryMs = 1000;
constexpr uint32_t kI2cMaxRetryMs = 30000;

enum class I2cFaultAction : uint8_t {
  None,
  Check,    // bus is up: probe the devices
  Recover   // bus is down: try to bring it back
};

// Decides when the I2C bus counts as down and when to try bringing it back.
// Pure logic: the caller does the bus work and reports each result.
class I2cFaultTracker {
public:
  I2cFaultAction due(uint32_t now) const;

  // Result of a health check while up. True when this check took the bus down.
  bool noteCheck(bool ok, uint32_t now);

  // Result of a recovery attempt while down. A failed attempt doubles the wait
  // before the next one, up to kI2cMaxRetryMs.
  void noteRecovery(bool ok, uint32_t now);

  bool down() const { return down_; }
  uint32_t faults() const { return faults_; }
  uint32_t recoveries() const { return recoveries_; }
  // millis() of the latest fault; 0 when there has been none.
  uint32_t lastFaultMs() const { return lastFaultMs_; }
  // Recovery attempts since the current fault began.
  uint32_t attempts() const { return attempts_; }
  uint32_t retryDelayMs() const { return retryDelayMs_; }

private:
  bool down_ = false;
  bool checked_ = false;
  uint8_t failures_ = 0;
  uint32_t lastCheckMs_ = 0;
  uint32_t lastAttemptMs_ = 0;
  uint32_t retryDelayMs_ = 0;
  uint32_t attempts_ = 0;
  uint32_t faults_ = 0;
  uint32_t recoveries_ = 0;
  uint32_t lastFaultMs_ = 0;
};
