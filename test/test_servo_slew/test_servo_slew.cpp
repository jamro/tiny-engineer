#include <unity.h>

#include "servos.h"

void test_slew_steps_at_speed() {
  // 100 deg/s for 10 ms is 1 deg.
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 91.0f, servoSlewNext(90.0f, 180.0f, 100.0f, 10));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 89.0f, servoSlewNext(90.0f, 15.0f, 100.0f, 10));
}

void test_slew_lands_on_target() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.2f, servoSlewNext(15.5f, 15.2f, 100.0f, 10));
}

void test_slew_caps_stalled_gap() {
  // A 3 s stall at 25 deg/s used to jump 75 deg in one step.
  const float next = servoSlewNext(15.0f, 90.0f, 25.0f, 3000);
  const float capped = 15.0f + 25.0f * SERVO_MAX_STEP_GAP_MS / 1000.0f;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, capped, next);
  TEST_ASSERT_TRUE(next < 20.0f);
}

void test_slew_zero_speed_holds() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 40.0f, servoSlewNext(40.0f, 90.0f, 0.0f, 50));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_slew_steps_at_speed);
  RUN_TEST(test_slew_lands_on_target);
  RUN_TEST(test_slew_caps_stalled_gap);
  RUN_TEST(test_slew_zero_speed_holds);
  return UNITY_END();
}
