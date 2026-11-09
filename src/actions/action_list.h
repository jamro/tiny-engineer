#pragma once

#include <cstddef>

#include "actions/action.h"

class ActionList {
public:
  size_t count() const;
  const ActionEntry& at(size_t index) const;
  const ActionEntry* data() const;

  // Appends a validated entry. False when invalid or the list is full.
  bool add(const ActionEntry& entry);

  // NVS blob: count() entries back to back. False (list unchanged) when the
  // blob is not a whole number of valid entries.
  bool load(const void* blob, size_t length);
  void clear();

private:
  ActionEntry entries_[ACTIONS_MAX] = {};
  size_t count_ = 0;
};
