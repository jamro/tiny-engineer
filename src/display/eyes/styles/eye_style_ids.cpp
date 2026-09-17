#include "display/eyes/styles/eye_style.h"

#include <cstring>

namespace {

constexpr const char* kEyeStyleIds[] = {
  "classic",
  "kaomoji",
};

}  // namespace

bool isRegisteredEyeStyleId(const char* id) {
  if (id == nullptr) {
    return false;
  }

  for (size_t i = 0; i < registeredEyeStyleCount(); i++) {
    if (strcmp(id, kEyeStyleIds[i]) == 0) {
      return true;
    }
  }

  return false;
}

size_t registeredEyeStyleCount() {
  return sizeof(kEyeStyleIds) / sizeof(kEyeStyleIds[0]);
}

const char* registeredEyeStyleIdAt(size_t index) {
  if (index >= registeredEyeStyleCount()) {
    return nullptr;
  }

  return kEyeStyleIds[index];
}
