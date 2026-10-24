#include <cstring>
#include <unity.h>

#include "extensions/pin_claims.h"

void test_push_button_claim_when_enabled() {
  ExtPinClaim claim{};
  TEST_ASSERT_TRUE(pinClaimPushButton(true, 20, claim));
  TEST_ASSERT_EQUAL_STRING("push-button", claim.id);
  TEST_ASSERT_EQUAL_UINT(1, claim.pinCount);
  TEST_ASSERT_EQUAL_UINT8(20, claim.pins[0]);
}

void test_push_button_no_claim_when_disabled() {
  ExtPinClaim claim{};
  TEST_ASSERT_FALSE(pinClaimPushButton(false, 20, claim));
  TEST_ASSERT_EQUAL_UINT(0, claim.pinCount);
}

void test_disabled_same_pin_no_conflict() {
  ExtPinClaim claims[2] = {};
  // Only enabled extensions produce claims; two disabled would yield count 0.
  TEST_ASSERT_FALSE(pinClaimsConflict(claims, 0));
  TEST_ASSERT_FALSE(pinClaimsConflict(claims, 1));
}

void test_two_enabled_same_pin_conflicts() {
  ExtPinClaim claims[2] = {};
  pinClaimPushButton(true, 20, claims[0]);
  claims[1].id = "other-ext";
  claims[1].pins[0] = 20;
  claims[1].pinCount = 1;

  char message[96];
  TEST_ASSERT_TRUE(pinClaimsConflict(claims, 2, message, sizeof(message)));
  TEST_ASSERT_NOT_NULL(strstr(message, "GP20"));
  TEST_ASSERT_NOT_NULL(strstr(message, "push-button"));
  TEST_ASSERT_NOT_NULL(strstr(message, "other-ext"));
  TEST_ASSERT_TRUE(pinClaimsPinIsContested(claims, 2, 20));
}

void test_two_enabled_different_pins_ok() {
  ExtPinClaim claims[2] = {};
  pinClaimPushButton(true, 20, claims[0]);
  claims[1].id = "other-ext";
  claims[1].pins[0] = 6;
  claims[1].pinCount = 1;

  TEST_ASSERT_FALSE(pinClaimsConflict(claims, 2));
  TEST_ASSERT_FALSE(pinClaimsPinIsContested(claims, 2, 20));
  TEST_ASSERT_FALSE(pinClaimsPinIsContested(claims, 2, 6));
}

void test_contested_pin_idle_helper() {
  ExtPinClaim claims[2] = {};
  pinClaimPushButton(true, 7, claims[0]);
  claims[1].id = "other-ext";
  claims[1].pins[0] = 7;
  claims[1].pinCount = 1;

  TEST_ASSERT_TRUE(pinClaimsPinIsContested(claims, 2, 7));
  TEST_ASSERT_FALSE(pinClaimsPinIsContested(claims, 2, 20));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_push_button_claim_when_enabled);
  RUN_TEST(test_push_button_no_claim_when_disabled);
  RUN_TEST(test_disabled_same_pin_no_conflict);
  RUN_TEST(test_two_enabled_same_pin_conflicts);
  RUN_TEST(test_two_enabled_different_pins_ok);
  RUN_TEST(test_contested_pin_idle_helper);
  return UNITY_END();
}
