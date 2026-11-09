#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t ACTIONS_MAX = 16;
constexpr size_t ACTIONS_ON_MAX_LEN = 47;
constexpr size_t ACTIONS_ANIM_MAX_LEN = 15;

enum class ActionDo : uint8_t {
  Sound = 1,
  Anim = 2,
};

enum class ActionSound : uint8_t {
  Bell = 0,
  Click = 1,
};

// Fixed layout for the NVS blob — keep fields stable across firmware versions.
struct ActionEntry {
  char on[ACTIONS_ON_MAX_LEN + 1];
  ActionDo doKind;
  ActionSound sound;
  char anim[ACTIONS_ANIM_MAX_LEN + 1];
};

bool actionsValidateEntry(const ActionEntry& entry);
bool actionsParseDo(const char* text, ActionDo& out);
bool actionsParseSound(const char* text, ActionSound& out);
const char* actionsDoName(ActionDo kind);
const char* actionsSoundName(ActionSound sound);
