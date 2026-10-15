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

void test_calibrated_center_preserves_safe_limits() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 82.5f, servoNormToDeg(0.0f, 70.0f, 82.5f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 76.25f, servoNormToDeg(-0.5f, 70.0f, 82.5f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 91.25f, servoNormToDeg(0.5f, 70.0f, 82.5f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 70.0f, servoNormToDeg(-1.0f, 70.0f, 82.5f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, servoNormToDeg(1.0f, 70.0f, 82.5f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 70.0f, servoNormToDeg(-2.0f, 70.0f, 82.5f, 100.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, servoNormToDeg(2.0f, 70.0f, 82.5f, 100.0f));
}

void test_calibrated_center_inverse_roundtrip_and_saturation() {
  for (int step = -10; step <= 10; step++) {
    const float n = step / 10.0f;
    const float deg = servoNormToDeg(n, 35.0f, 110.5f, 125.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, n, servoDegToNorm(deg, 35.0f, 110.5f, 125.0f));
  }

  TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, servoDegToNorm(0.0f, 35.0f, 110.5f, 125.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, servoDegToNorm(180.0f, 35.0f, 110.5f, 125.0f));
}

void test_default_centers_keep_legacy_mapping() {
  for (const ServoSpec& spec : SERVO_SPECS) {
    for (int step = -10; step <= 10; step++) {
      const float n = step / 10.0f;
      const float expected = servoMid(spec) + n * (spec.max - spec.min) * 0.5f;
      TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, servoNormToDeg(n, spec.min, servoMid(spec), spec.max));
    }
  }
}

void test_hand_assignment_swaps_only_hand_channels() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    TEST_ASSERT_EQUAL_INT(SERVO_SPECS[i].channel, servoChannelFor(i, false));
  }

  TEST_ASSERT_EQUAL_INT(3, servoChannelFor(SERVO_HAND_LEFT, true));
  TEST_ASSERT_EQUAL_INT(2, servoChannelFor(SERVO_HAND_RIGHT, true));
  TEST_ASSERT_EQUAL_INT(0, servoChannelFor(SERVO_HEAD, true));
  TEST_ASSERT_EQUAL_INT(1, servoChannelFor(SERVO_NECK, true));
  TEST_ASSERT_EQUAL_INT(4, servoChannelFor(SERVO_BODY, true));
  TEST_ASSERT_EQUAL_INT(-1, servoChannelFor(-1, true));
  TEST_ASSERT_EQUAL_INT(-1, servoChannelFor(SERVO_COUNT, false));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_norm_mid_is_average);
  RUN_TEST(test_norm_ends);
  RUN_TEST(test_norm_saturates);
  RUN_TEST(test_deg_to_norm_roundtrip);
  RUN_TEST(test_stock_head_identity);
  RUN_TEST(test_calibrated_center_preserves_safe_limits);
  RUN_TEST(test_calibrated_center_inverse_roundtrip_and_saturation);
  RUN_TEST(test_default_centers_keep_legacy_mapping);
  RUN_TEST(test_hand_assignment_swaps_only_hand_channels);
  return UNITY_END();
}
