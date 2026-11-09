#pragma once

#include "actions/action_list.h"

// When actions is empty, append sound rows for each non-none legacy button SFX.
// Returns true when any row was added.
bool actionsMigrateLegacyButtonSounds(
  ActionList& actions,
  const char* soundDown,
  const char* soundUp,
  const char* soundClick,
  const char* soundDoubleClick,
  const char* soundLongClick
);
