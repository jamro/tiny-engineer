#include <unity.h>

#include "actions/pattern.h"

void test_exact_match() {
  TEST_ASSERT_TRUE(actionsPatternMatch("button.click", "button.click"));
  TEST_ASSERT_FALSE(actionsPatternMatch("button.click", "button.up"));
}

void test_suffix_wildcard() {
  TEST_ASSERT_TRUE(actionsPatternMatch("button.*", "button.click"));
  TEST_ASSERT_TRUE(actionsPatternMatch("button.*", "button.double_click"));
  TEST_ASSERT_FALSE(actionsPatternMatch("button.*", "led.on"));
}

void test_prefix_wildcard() {
  TEST_ASSERT_TRUE(actionsPatternMatch("*.click", "button.click"));
  TEST_ASSERT_FALSE(actionsPatternMatch("*.click", "button.up"));
}

void test_mid_wildcard() {
  TEST_ASSERT_TRUE(actionsPatternMatch("*.double_*", "button.double_click"));
  TEST_ASSERT_FALSE(actionsPatternMatch("*.double_*", "button.click"));
}

void test_multi_star() {
  TEST_ASSERT_TRUE(actionsPatternMatch("*button*", "xbutton.y"));
  TEST_ASSERT_TRUE(actionsPatternMatch("**", "anything"));
  TEST_ASSERT_TRUE(actionsPatternMatch("*", "button.click"));
}

void test_empty_type_only_star() {
  TEST_ASSERT_TRUE(actionsPatternMatch("*", ""));
  TEST_ASSERT_FALSE(actionsPatternMatch("a", ""));
  TEST_ASSERT_FALSE(actionsPatternMatch("", "a"));
  TEST_ASSERT_TRUE(actionsPatternMatch("", ""));
}

void test_null_rejected() {
  TEST_ASSERT_FALSE(actionsPatternMatch(nullptr, "button.click"));
  TEST_ASSERT_FALSE(actionsPatternMatch("button.click", nullptr));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_exact_match);
  RUN_TEST(test_suffix_wildcard);
  RUN_TEST(test_prefix_wildcard);
  RUN_TEST(test_mid_wildcard);
  RUN_TEST(test_multi_star);
  RUN_TEST(test_empty_type_only_star);
  RUN_TEST(test_null_rejected);
  return UNITY_END();
}
