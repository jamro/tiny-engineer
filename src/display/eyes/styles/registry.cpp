#include "display/eyes/styles/eye_style.h"

#include <cstring>

#include "settings/settings.h"

extern const EyeStyleRenderer kClassicEyeStyle;
extern const EyeStyleRenderer kKaomojiEyeStyle;
extern const EyeStyleRenderer kCoverEyeStyle;
extern const EyeStyleRenderer kDotsEyeStyle;

namespace {

const EyeStyleRenderer* const kEyeStyles[] = {
  &kClassicEyeStyle,
  &kKaomojiEyeStyle,
  &kCoverEyeStyle,
  &kDotsEyeStyle,
};

}  // namespace

const EyeStyleRenderer* findEyeStyle(const char* id) {
  if (id != nullptr) {
    for (const EyeStyleRenderer* style : kEyeStyles) {
      if (strcmp(id, style->id) == 0) {
        return style;
      }
    }
  }

  return &kClassicEyeStyle;
}

const EyeStyleRenderer* currentEyeStyle() {
  return findEyeStyle(settingsEyesStyle());
}
