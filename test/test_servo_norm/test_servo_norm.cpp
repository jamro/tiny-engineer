#include <unity.h>

#include "servos.h"

void test_norm_mid_is_average() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 85.0f, servoNormToDeg(0.0f, 70.0f, 100.0f));
}

void test_norm_ends() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 70.0f, servoNormToDeg(-1.0f, 70.0f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, servoNormToDeg(1.0f, 70.0f, 100.0f));
}

void test_norm_saturates() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 70.0f, servoNormToDeg(-2.0f, 70.0f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, servoNormToDeg(2.0f, 70.0f, 100.0f));
}

void test_deg_to_norm_roundtrip() {
  const float minDeg = 70.0f;
  const float maxDeg = 100.0f;
  const float n = servoDegToNorm(85.0f, minDeg, maxDeg);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, n);
  TEST_ASSERT_FLOAT_WITHIN(
    0.001f,
    85.0f,
    servoNormToDeg(n, minDeg, maxDeg)
  );
}

void test_stock_head_identity() {
  const float minDeg = SERVO_SPECS[SERVO_HEAD].min;
  const float maxDeg = SERVO_SPECS[SERVO_HEAD].max;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, minDeg, servoNormToDeg(-1.0f, minDeg, maxDeg));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, maxDeg, servoNormToDeg(1.0f, minDeg, maxDeg));
  TEST_ASSERT_FLOAT_WITHIN(
    0.001f,
    servoMid(SERVO_SPECS[SERVO_HEAD]),
    servoNormToDeg(0.0f, minDeg, maxDeg)
  );
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_norm_mid_is_average);
  RUN_TEST(test_norm_ends);
  RUN_TEST(test_norm_saturates);
  RUN_TEST(test_deg_to_norm_roundtrip);
  RUN_TEST(test_stock_head_identity);
  return UNITY_END();
}
