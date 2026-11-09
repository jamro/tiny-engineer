#include "actions/pattern.h"

#include <cstring>

bool actionsPatternMatch(const char* pattern, const char* type) {
  if (pattern == nullptr || type == nullptr) {
    return false;
  }

  const char* p = pattern;
  const char* t = type;

  while (*p != '\0' && *t != '\0') {
    if (*p == '*') {
      // Collapse consecutive stars.
      while (p[1] == '*') {
        ++p;
      }
      if (p[1] == '\0') {
        return true;
      }
      // Try matching the rest starting at each remaining type position.
      for (; *t != '\0'; ++t) {
        if (actionsPatternMatch(p + 1, t)) {
          return true;
        }
      }
      return actionsPatternMatch(p + 1, t);
    }

    if (*p != *t) {
      return false;
    }
    ++p;
    ++t;
  }

  while (*p == '*') {
    ++p;
  }

  return *p == '\0' && *t == '\0';
}
