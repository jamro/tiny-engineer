#include "actions/migrate.h"

#include <cstring>

namespace {

bool appendSound(ActionList& actions, const char* on, const char* sound) {
  ActionSound clip;
  if (!actionsParseSound(sound, clip)) {
    return false;
  }

  ActionEntry entry{};
  strncpy(entry.on, on, ACTIONS_ON_MAX_LEN);
  entry.on[ACTIONS_ON_MAX_LEN] = '\0';
  entry.doKind = ActionDo::Sound;
  entry.sound = clip;
  return actions.add(entry);
}

}  // namespace

bool actionsMigrateLegacyButtonSounds(
  ActionList& actions,
  const char* soundDown,
  const char* soundUp,
  const char* soundClick,
  const char* soundDoubleClick,
  const char* soundLongClick
) {
  if (actions.count() != 0) {
    return false;
  }

  bool added = false;
  added |= appendSound(actions, "button.down", soundDown);
  added |= appendSound(actions, "button.up", soundUp);
  added |= appendSound(actions, "button.click", soundClick);
  added |= appendSound(actions, "button.double_click", soundDoubleClick);
  added |= appendSound(actions, "button.long_click", soundLongClick);
  return added;
}
