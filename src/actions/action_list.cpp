#include "actions/action_list.h"

#include <algorithm>
#include <cstring>

namespace {

bool terminated(const char* text, size_t size) {
  return memchr(text, '\0', size) != nullptr;
}

bool validStoredEntry(const ActionEntry& entry) {
  if (!terminated(entry.on, sizeof(entry.on)) ||
      !terminated(entry.anim, sizeof(entry.anim))) {
    return false;
  }
  return actionsValidateEntry(entry);
}

}  // namespace

size_t ActionList::count() const {
  return count_;
}

const ActionEntry& ActionList::at(size_t index) const {
  return entries_[index];
}

const ActionEntry* ActionList::data() const {
  return entries_;
}

bool ActionList::add(const ActionEntry& entry) {
  if (!actionsValidateEntry(entry) || count_ == ACTIONS_MAX) {
    return false;
  }

  entries_[count_++] = entry;
  return true;
}

bool ActionList::load(const void* blob, size_t length) {
  const size_t loadedCount = length / sizeof(ActionEntry);

  if (length % sizeof(ActionEntry) != 0 || loadedCount > ACTIONS_MAX) {
    return false;
  }

  const auto* loaded = static_cast<const ActionEntry*>(blob);

  if (!std::all_of(loaded, loaded + loadedCount, validStoredEntry)) {
    return false;
  }

  clear();
  std::copy(loaded, loaded + loadedCount, entries_);
  count_ = loadedCount;
  return true;
}

void ActionList::clear() {
  std::fill(entries_, entries_ + ACTIONS_MAX, ActionEntry{});
  count_ = 0;
}
