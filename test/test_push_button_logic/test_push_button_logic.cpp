#include <unity.h>

#include "extensions/push_button_logic.h"
#include "extensions/push_button_sfx.h"

namespace {

void expectEvent(std::optional<PushButtonEvent> got, PushButtonEvent want) {
  TEST_ASSERT_TRUE(got.has_value());
  TEST_ASSERT_EQUAL(static_cast<int>(want), static_cast<int>(*got));
}

}  // namespace

void test_enable_samples_without_event() {
  PushButtonLogic logic;
  logic.apply(true, {false, 1000});  // held low at enable
  TEST_ASSERT_FALSE(logic.accept({false, 2000}).has_value());
  // Release after enable-while-held stays silent.
  TEST_ASSERT_FALSE(logic.accept({true, 1000 + kPushButtonDebounceUs}).has_value());
  // Next press is the first event.
  expectEvent(logic.accept({false, 1000 + 2 * kPushButtonDebounceUs}), PushButtonEvent::Down);
}

void test_press_and_release() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  expectEvent(logic.accept({false, kPushButtonDebounceUs}), PushButtonEvent::Down);
  expectEvent(logic.accept({true, 2 * kPushButtonDebounceUs}), PushButtonEvent::Up);
}

void test_bounce_suppressed() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  TEST_ASSERT_TRUE(logic.accept({false, kPushButtonDebounceUs}).has_value());
  // Bounce back high too soon — ignored.
  TEST_ASSERT_FALSE(logic.accept({true, kPushButtonDebounceUs + 1000}).has_value());
  TEST_ASSERT_FALSE(logic.accept({false, kPushButtonDebounceUs + 2000}).has_value());
  // Stable release after debounce from last accepted (the press).
  expectEvent(logic.accept({true, 2 * kPushButtonDebounceUs}), PushButtonEvent::Up);
}

void test_queued_click_while_poll_paused() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  // Edges arrived while polling was paused (e.g. during POST /play).
  expectEvent(logic.accept({false, 30ULL * 1000ULL}), PushButtonEvent::Down);
  expectEvent(logic.accept({true, 60ULL * 1000ULL}), PushButtonEvent::Up);
}

void test_disable_stops_events() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});
  logic.accept({false, kPushButtonDebounceUs});
  logic.apply(false, {});
  TEST_ASSERT_FALSE(logic.accept({true, 2 * kPushButtonDebounceUs}).has_value());
  TEST_ASSERT_FALSE(logic.tick(2 * kPushButtonDebounceUs + kPushButtonLongHoldUs).has_value());
}

void test_delayed_click() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  const uint64_t t0 = kPushButtonDebounceUs;
  const uint64_t tUp = t0 + 100ULL * 1000ULL;  // 100 ms hold
  expectEvent(logic.accept({false, t0}), PushButtonEvent::Down);
  expectEvent(logic.accept({true, tUp}), PushButtonEvent::Up);
  TEST_ASSERT_FALSE(logic.tick(tUp + kPushButtonDoubleWaitUs - 1).has_value());
  expectEvent(logic.tick(tUp + kPushButtonDoubleWaitUs), PushButtonEvent::Click);
  TEST_ASSERT_FALSE(logic.tick(tUp + kPushButtonDoubleWaitUs + 1).has_value());
}

void test_double_click_no_single_click() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  const uint64_t d1 = kPushButtonDebounceUs;
  const uint64_t u1 = d1 + 80ULL * 1000ULL;
  const uint64_t d2 = u1 + 100ULL * 1000ULL;  // within 350 ms
  const uint64_t u2 = d2 + 80ULL * 1000ULL;

  expectEvent(logic.accept({false, d1}), PushButtonEvent::Down);
  expectEvent(logic.accept({true, u1}), PushButtonEvent::Up);
  expectEvent(logic.accept({false, d2}), PushButtonEvent::Down);
  expectEvent(logic.accept({true, u2}), PushButtonEvent::Up);
  expectEvent(logic.tick(u2), PushButtonEvent::DoubleClick);
  TEST_ASSERT_FALSE(logic.tick(u2 + kPushButtonDoubleWaitUs).has_value());
}

void test_long_click_on_hold_then_up_only() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  const uint64_t d = kPushButtonDebounceUs;
  expectEvent(logic.accept({false, d}), PushButtonEvent::Down);
  TEST_ASSERT_FALSE(logic.tick(d + kPushButtonLongHoldUs - 1).has_value());
  expectEvent(logic.tick(d + kPushButtonLongHoldUs), PushButtonEvent::LongClick);
  expectEvent(logic.accept({true, d + kPushButtonLongHoldUs + 50ULL * 1000ULL}), PushButtonEvent::Up);
  TEST_ASSERT_FALSE(logic.tick(d + kPushButtonLongHoldUs + kPushButtonDoubleWaitUs).has_value());
}

void test_second_press_long_cancels_click_and_double() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  const uint64_t d1 = kPushButtonDebounceUs;
  const uint64_t u1 = d1 + 80ULL * 1000ULL;
  const uint64_t d2 = u1 + 100ULL * 1000ULL;

  expectEvent(logic.accept({false, d1}), PushButtonEvent::Down);
  expectEvent(logic.accept({true, u1}), PushButtonEvent::Up);
  expectEvent(logic.accept({false, d2}), PushButtonEvent::Down);
  expectEvent(logic.tick(d2 + kPushButtonLongHoldUs), PushButtonEvent::LongClick);
  expectEvent(logic.accept({true, d2 + kPushButtonLongHoldUs + 10ULL * 1000ULL}), PushButtonEvent::Up);
  TEST_ASSERT_FALSE(logic.tick(d2 + kPushButtonLongHoldUs + kPushButtonDoubleWaitUs).has_value());
}

void test_disable_clears_pending_click() {
  PushButtonLogic logic;
  logic.apply(true, {true, 0});

  const uint64_t d = kPushButtonDebounceUs;
  const uint64_t u = d + 50ULL * 1000ULL;
  expectEvent(logic.accept({false, d}), PushButtonEvent::Down);
  expectEvent(logic.accept({true, u}), PushButtonEvent::Up);
  logic.apply(false, {});
  TEST_ASSERT_FALSE(logic.tick(u + kPushButtonDoubleWaitUs).has_value());
}

void test_button_sfx_parse_and_busy_gate() {
  TEST_ASSERT_EQUAL(
    static_cast<int>(ButtonSfxClip::None),
    static_cast<int>(parseButtonSfx("none"))
  );
  TEST_ASSERT_EQUAL(
    static_cast<int>(ButtonSfxClip::Bell),
    static_cast<int>(parseButtonSfx("bell"))
  );
  TEST_ASSERT_EQUAL(
    static_cast<int>(ButtonSfxClip::Click),
    static_cast<int>(parseButtonSfx("click"))
  );
  TEST_ASSERT_EQUAL(
    static_cast<int>(ButtonSfxClip::None),
    static_cast<int>(parseButtonSfx("welcome"))
  );
  TEST_ASSERT_FALSE(shouldStartButtonSfx(ButtonSfxClip::None, false, false));
  TEST_ASSERT_TRUE(shouldStartButtonSfx(ButtonSfxClip::Bell, false, false));
  TEST_ASSERT_TRUE(shouldStartButtonSfx(ButtonSfxClip::Click, false, false));
  TEST_ASSERT_FALSE(shouldStartButtonSfx(ButtonSfxClip::Bell, true, false));
  TEST_ASSERT_FALSE(shouldStartButtonSfx(ButtonSfxClip::Click, false, true));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_enable_samples_without_event);
  RUN_TEST(test_press_and_release);
  RUN_TEST(test_bounce_suppressed);
  RUN_TEST(test_queued_click_while_poll_paused);
  RUN_TEST(test_disable_stops_events);
  RUN_TEST(test_delayed_click);
  RUN_TEST(test_double_click_no_single_click);
  RUN_TEST(test_long_click_on_hold_then_up_only);
  RUN_TEST(test_second_press_long_cancels_click_and_double);
  RUN_TEST(test_disable_clears_pending_click);
  RUN_TEST(test_button_sfx_parse_and_busy_gate);
  return UNITY_END();
}
