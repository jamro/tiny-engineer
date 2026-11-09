#include <cstdio>
#include <cstring>
#include <unity.h>

#include "actions/action_list.h"
#include "actions/migrate.h"

void test_add_and_round_trip() {
  ActionList list;
  ActionEntry sound{};
  strncpy(sound.on, "button.click", ACTIONS_ON_MAX_LEN);
  sound.doKind = ActionDo::Sound;
  sound.sound = ActionSound::Bell;

  ActionEntry anim{};
  strncpy(anim.on, "button.*", ACTIONS_ON_MAX_LEN);
  anim.doKind = ActionDo::Anim;
  strncpy(anim.anim, "welcome", ACTIONS_ANIM_MAX_LEN);

  TEST_ASSERT_TRUE(list.add(sound));
  TEST_ASSERT_TRUE(list.add(anim));
  TEST_ASSERT_EQUAL(2, list.count());

  ActionList loaded;
  TEST_ASSERT_TRUE(
    loaded.load(list.data(), list.count() * sizeof(ActionEntry))
  );
  TEST_ASSERT_EQUAL(2, loaded.count());
  TEST_ASSERT_EQUAL_STRING("button.click", loaded.at(0).on);
  TEST_ASSERT_EQUAL(static_cast<int>(ActionDo::Sound), static_cast<int>(loaded.at(0).doKind));
  TEST_ASSERT_EQUAL_STRING("welcome", loaded.at(1).anim);
}

void test_rejects_invalid_and_overflow() {
  ActionList list;
  ActionEntry bad{};
  bad.doKind = ActionDo::Sound;
  bad.sound = ActionSound::Click;
  TEST_ASSERT_FALSE(list.add(bad));  // empty on

  ActionEntry sound{};
  strncpy(sound.on, "button.up", ACTIONS_ON_MAX_LEN);
  sound.doKind = ActionDo::Sound;
  sound.sound = ActionSound::Click;

  for (size_t i = 0; i < ACTIONS_MAX; i++) {
    ActionEntry row = sound;
    snprintf(row.on, sizeof(row.on), "evt.%zu", i);
    TEST_ASSERT_TRUE(list.add(row));
  }
  TEST_ASSERT_FALSE(list.add(sound));
  TEST_ASSERT_EQUAL(ACTIONS_MAX, list.count());
}

void test_migrate_empty_list() {
  ActionList actions;
  TEST_ASSERT_TRUE(actionsMigrateLegacyButtonSounds(
    actions, "none", "none", "bell", "none", "click"
  ));
  TEST_ASSERT_EQUAL(2, actions.count());
  TEST_ASSERT_EQUAL_STRING("button.click", actions.at(0).on);
  TEST_ASSERT_EQUAL(static_cast<int>(ActionSound::Bell), static_cast<int>(actions.at(0).sound));
  TEST_ASSERT_EQUAL_STRING("button.long_click", actions.at(1).on);
  TEST_ASSERT_EQUAL(static_cast<int>(ActionSound::Click), static_cast<int>(actions.at(1).sound));
}

void test_migrate_skips_when_actions_present() {
  ActionList actions;
  ActionEntry row{};
  strncpy(row.on, "button.click", ACTIONS_ON_MAX_LEN);
  row.doKind = ActionDo::Sound;
  row.sound = ActionSound::Click;
  TEST_ASSERT_TRUE(actions.add(row));

  TEST_ASSERT_FALSE(actionsMigrateLegacyButtonSounds(
    actions, "bell", "bell", "bell", "bell", "bell"
  ));
  TEST_ASSERT_EQUAL(1, actions.count());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_add_and_round_trip);
  RUN_TEST(test_rejects_invalid_and_overflow);
  RUN_TEST(test_migrate_empty_list);
  RUN_TEST(test_migrate_skips_when_actions_present);
  return UNITY_END();
}
