#include "actions/action.h"

#include <cstring>

namespace {

bool terminated(const char* text, size_t size) {
  return memchr(text, '\0', size) != nullptr;
}

bool nonEmptyTerminated(const char* text, size_t size, size_t maxLen) {
  return text != nullptr && terminated(text, size) && text[0] != '\0' &&
         strlen(text) <= maxLen;
}

}  // namespace

bool actionsParseDo(const char* text, ActionDo& out) {
  if (text == nullptr) {
    return false;
  }
  if (strcmp(text, "sound") == 0) {
    out = ActionDo::Sound;
    return true;
  }
  if (strcmp(text, "anim") == 0) {
    out = ActionDo::Anim;
    return true;
  }
  return false;
}

bool actionsParseSound(const char* text, ActionSound& out) {
  if (text == nullptr) {
    return false;
  }
  if (strcmp(text, "bell") == 0) {
    out = ActionSound::Bell;
    return true;
  }
  if (strcmp(text, "click") == 0) {
    out = ActionSound::Click;
    return true;
  }
  return false;
}

const char* actionsDoName(ActionDo kind) {
  switch (kind) {
    case ActionDo::Sound:
      return "sound";
    case ActionDo::Anim:
      return "anim";
  }
  return "";
}

const char* actionsSoundName(ActionSound sound) {
  switch (sound) {
    case ActionSound::Bell:
      return "bell";
    case ActionSound::Click:
      return "click";
  }
  return "";
}

bool actionsValidateEntry(const ActionEntry& entry) {
  if (!nonEmptyTerminated(entry.on, sizeof(entry.on), ACTIONS_ON_MAX_LEN)) {
    return false;
  }

  switch (entry.doKind) {
    case ActionDo::Sound:
      return entry.sound == ActionSound::Bell || entry.sound == ActionSound::Click;
    case ActionDo::Anim:
      return nonEmptyTerminated(entry.anim, sizeof(entry.anim), ACTIONS_ANIM_MAX_LEN);
  }
  return false;
}
