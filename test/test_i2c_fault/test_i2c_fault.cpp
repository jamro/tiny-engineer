#include <unity.h>

#include "hardware/i2c_fault.h"

namespace {

void expectAction(I2cFaultAction want, I2cFaultAction got) {
  TEST_ASSERT_EQUAL(static_cast<int>(want), static_cast<int>(got));
}

// Fails enough checks in a row to take the bus down; returns the time of the last.
uint32_t takeDown(I2cFaultTracker& tracker, uint32_t start) {
  uint32_t now = start;
  for (uint8_t i = 0; i < kI2cFailuresToFault; i++) {
    now = start + i * kI2cCheckIntervalMs;
    tracker.noteCheck(false, now);
  }
  return now;
}

}  // namespace

void test_checks_on_first_call_then_on_interval() {
  I2cFaultTracker tracker;
  expectAction(I2cFaultAction::Check, tracker.due(0));

  tracker.noteCheck(true, 100);
  expectAction(I2cFaultAction::None, tracker.due(100 + kI2cCheckIntervalMs - 1));
  expectAction(I2cFaultAction::Check, tracker.due(100 + kI2cCheckIntervalMs));
}

void test_one_failed_check_is_not_a_fault() {
  I2cFaultTracker tracker;
  TEST_ASSERT_FALSE(tracker.noteCheck(false, 1000));
  TEST_ASSERT_FALSE(tracker.down());
  TEST_ASSERT_EQUAL_UINT32(0, tracker.faults());

  // A good check in between resets the count.
  tracker.noteCheck(true, 1500);
  TEST_ASSERT_FALSE(tracker.noteCheck(false, 2000));
  TEST_ASSERT_FALSE(tracker.down());
}

void test_consecutive_failures_take_the_bus_down() {
  I2cFaultTracker tracker;
  TEST_ASSERT_FALSE(tracker.noteCheck(false, 1000));
  TEST_ASSERT_TRUE(tracker.noteCheck(false, 1500));
  TEST_ASSERT_TRUE(tracker.down());
  TEST_ASSERT_EQUAL_UINT32(1, tracker.faults());
  TEST_ASSERT_EQUAL_UINT32(1500, tracker.lastFaultMs());
}

void test_first_recovery_attempt_is_immediate() {
  I2cFaultTracker tracker;
  const uint32_t at = takeDown(tracker, 1000);
  expectAction(I2cFaultAction::Recover, tracker.due(at));
}

void test_failed_recovery_backs_off_to_the_cap() {
  I2cFaultTracker tracker;
  uint32_t now = takeDown(tracker, 1000);

  tracker.noteRecovery(false, now);
  TEST_ASSERT_EQUAL_UINT32(kI2cFirstRetryMs, tracker.retryDelayMs());
  expectAction(I2cFaultAction::None, tracker.due(now + kI2cFirstRetryMs - 1));
  expectAction(I2cFaultAction::Recover, tracker.due(now + kI2cFirstRetryMs));

  uint32_t previous = tracker.retryDelayMs();
  for (int i = 0; i < 12; i++) {
    now += tracker.retryDelayMs();
    tracker.noteRecovery(false, now);
    TEST_ASSERT_TRUE(tracker.retryDelayMs() >= previous);
    TEST_ASSERT_TRUE(tracker.retryDelayMs() <= kI2cMaxRetryMs);
    previous = tracker.retryDelayMs();
  }

  TEST_ASSERT_EQUAL_UINT32(kI2cMaxRetryMs, tracker.retryDelayMs());
  TEST_ASSERT_TRUE(tracker.down());
  TEST_ASSERT_EQUAL_UINT32(13, tracker.attempts());
  TEST_ASSERT_EQUAL_UINT32(0, tracker.recoveries());
}

void test_recovery_brings_the_bus_up_and_resumes_checks() {
  I2cFaultTracker tracker;
  const uint32_t at = takeDown(tracker, 1000);

  tracker.noteRecovery(false, at);
  tracker.noteRecovery(true, at + kI2cFirstRetryMs);

  TEST_ASSERT_FALSE(tracker.down());
  TEST_ASSERT_EQUAL_UINT32(1, tracker.recoveries());
  TEST_ASSERT_EQUAL_UINT32(2, tracker.attempts());
  expectAction(I2cFaultAction::None, tracker.due(at + kI2cFirstRetryMs));
  expectAction(
    I2cFaultAction::Check,
    tracker.due(at + kI2cFirstRetryMs + kI2cCheckIntervalMs)
  );
}

void test_second_fault_starts_over() {
  I2cFaultTracker tracker;
  uint32_t now = takeDown(tracker, 1000);
  tracker.noteRecovery(false, now);
  tracker.noteRecovery(false, now + kI2cFirstRetryMs);
  tracker.noteRecovery(true, now + 3 * kI2cFirstRetryMs);

  now = takeDown(tracker, 60000);
  TEST_ASSERT_TRUE(tracker.down());
  TEST_ASSERT_EQUAL_UINT32(2, tracker.faults());
  TEST_ASSERT_EQUAL_UINT32(0, tracker.attempts());
  // Back to an immediate first attempt, not the old backoff.
  expectAction(I2cFaultAction::Recover, tracker.due(now));
}

void test_a_fault_takes_the_bus_down_at_once() {
  I2cFaultTracker tracker;
  tracker.noteCheck(true, 1000);

  TEST_ASSERT_TRUE(tracker.noteFault(1200));
  TEST_ASSERT_TRUE(tracker.down());
  TEST_ASSERT_EQUAL_UINT32(1, tracker.faults());
  TEST_ASSERT_EQUAL_UINT32(1200, tracker.lastFaultMs());
  expectAction(I2cFaultAction::Recover, tracker.due(1200));
}

void test_a_fault_while_down_is_not_counted_again() {
  I2cFaultTracker tracker;
  const uint32_t at = takeDown(tracker, 1000);
  tracker.noteRecovery(false, at);

  TEST_ASSERT_FALSE(tracker.noteFault(at + 10));
  TEST_ASSERT_EQUAL_UINT32(1, tracker.faults());
  TEST_ASSERT_EQUAL_UINT32(at, tracker.lastFaultMs());
  // The backoff the failed attempt started still holds.
  expectAction(I2cFaultAction::None, tracker.due(at + kI2cFirstRetryMs - 1));
}

void test_a_fault_before_the_first_check_still_recovers() {
  I2cFaultTracker tracker;
  TEST_ASSERT_TRUE(tracker.noteFault(50));
  expectAction(I2cFaultAction::Recover, tracker.due(50));

  tracker.noteRecovery(true, 60);
  TEST_ASSERT_FALSE(tracker.down());
  TEST_ASSERT_EQUAL_UINT32(1, tracker.recoveries());
}

void test_intervals_survive_millis_wrap() {
  I2cFaultTracker tracker;
  const uint32_t beforeWrap = UINT32_MAX - 100;
  tracker.noteCheck(true, beforeWrap);
  expectAction(I2cFaultAction::None, tracker.due(beforeWrap + 200));
  expectAction(I2cFaultAction::Check, tracker.due(beforeWrap + kI2cCheckIntervalMs));

  takeDown(tracker, beforeWrap + kI2cCheckIntervalMs);
  const uint32_t failedAt = beforeWrap + 2 * kI2cCheckIntervalMs;
  tracker.noteRecovery(false, failedAt);
  expectAction(I2cFaultAction::None, tracker.due(failedAt + kI2cFirstRetryMs - 1));
  expectAction(I2cFaultAction::Recover, tracker.due(failedAt + kI2cFirstRetryMs));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_checks_on_first_call_then_on_interval);
  RUN_TEST(test_one_failed_check_is_not_a_fault);
  RUN_TEST(test_consecutive_failures_take_the_bus_down);
  RUN_TEST(test_first_recovery_attempt_is_immediate);
  RUN_TEST(test_failed_recovery_backs_off_to_the_cap);
  RUN_TEST(test_recovery_brings_the_bus_up_and_resumes_checks);
  RUN_TEST(test_second_fault_starts_over);
  RUN_TEST(test_a_fault_takes_the_bus_down_at_once);
  RUN_TEST(test_a_fault_while_down_is_not_counted_again);
  RUN_TEST(test_a_fault_before_the_first_check_still_recovers);
  RUN_TEST(test_intervals_survive_millis_wrap);
  return UNITY_END();
}
