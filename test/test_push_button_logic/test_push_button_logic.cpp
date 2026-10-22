#include <unity.h>

#include "extensions/push_button_logic.h"

void test_enable_samples_without_event() {
  PushButtonLogic logic;
  logic.apply(true, {false, 1000});  // held low at enable
  TEST_ASSERT_FALSE(logic.accept({false, 2000}).has_value());
  // Release after enable-while-held stays silent.
  TEST_ASSERT_FALSE(logic.accept({true, 1000 + kPushButtonDebounceUs}).has_value());
  // Next press is the first event.
  const auto down = logic.accept({false, 1000 + 2 * kPushButtonDebounceUs});
  TEST_ASSERT_TRUE(down.has_value());
  TEST_ASSERT_EQUAL(static_cast<int>(PushButtonEvent::Down), static_cast<int>(*down));
}

void test_press_and_release() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  const auto down = logic.accept({false, kPushButtonDebounceUs});
  TEST_ASSERT_TRUE(down.has_value());
  TEST_ASSERT_EQUAL(static_cast<int>(PushButtonEvent::Down), static_cast<int>(*down));

  const auto up = logic.accept({true, 2 * kPushButtonDebounceUs});
  TEST_ASSERT_TRUE(up.has_value());
  TEST_ASSERT_EQUAL(static_cast<int>(PushButtonEvent::Up), static_cast<int>(*up));
}

void test_bounce_suppressed() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  TEST_ASSERT_TRUE(logic.accept({false, kPushButtonDebounceUs}).has_value());
  // Bounce back high too soon — ignored.
  TEST_ASSERT_FALSE(logic.accept({true, kPushButtonDebounceUs + 1000}).has_value());
  // Still bouncing low — ignored (same as last accepted is low? wait last accepted is low)
  // After rejected high, lastAccepted is still low. Another low is same level — ignored.
  TEST_ASSERT_FALSE(logic.accept({false, kPushButtonDebounceUs + 2000}).has_value());
  // Stable release after debounce from last accepted (the press).
  const auto up = logic.accept({true, 2 * kPushButtonDebounceUs});
  TEST_ASSERT_TRUE(up.has_value());
  TEST_ASSERT_EQUAL(static_cast<int>(PushButtonEvent::Up), static_cast<int>(*up));
}

void test_queued_click_while_poll_paused() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  // Edges arrived while polling was paused (e.g. during POST /play).
  const auto down = logic.accept({false, 30ULL * 1000ULL});
  const auto up = logic.accept({true, 60ULL * 1000ULL});
  TEST_ASSERT_TRUE(down.has_value());
  TEST_ASSERT_TRUE(up.has_value());
  TEST_ASSERT_EQUAL(static_cast<int>(PushButtonEvent::Down), static_cast<int>(*down));
  TEST_ASSERT_EQUAL(static_cast<int>(PushButtonEvent::Up), static_cast<int>(*up));
}

void test_disable_stops_events() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});
  logic.accept({false, kPushButtonDebounceUs});
  logic.apply(false, {});
  TEST_ASSERT_FALSE(logic.accept({true, 2 * kPushButtonDebounceUs}).has_value());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_enable_samples_without_event);
  RUN_TEST(test_press_and_release);
  RUN_TEST(test_bounce_suppressed);
  RUN_TEST(test_queued_click_while_poll_paused);
  RUN_TEST(test_disable_stops_events);
  return UNITY_END();
}
