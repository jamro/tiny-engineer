#include "display/eyes/core/impact.h"

#include "animation/util.h"
#include "display/eyes/core/internal.h"
#include "display/eyes/core/util.h"

namespace {

bool g_impactActive = false;
uint32_t g_impactStartMs = 0;
uint32_t g_impactDurationMs = 0;
int8_t g_impactJitterX = 0;
int8_t g_impactJitterY = 0;

}  // namespace

void resetImpactState() {
  g_impactActive = false;
  g_impactStartMs = 0;
  g_impactDurationMs = 0;
  g_impactJitterX = 0;
  g_impactJitterY = 0;
}

void applyImpactOverlay(Eye& left, Eye& right, uint32_t now) {
  if (!g_impactActive) {
    return;
  }

  const float t = eyes::moveProgress(now, g_impactStartMs, g_impactDurationMs);

  if (t >= 1.0f) {
    g_impactActive = false;
    return;
  }

  left = eyes::eyeWithHeight(
    (int16_t)(left.x + g_impactJitterX),
    left.width,
    18
  );
  right = eyes::eyeWithHeight(
    (int16_t)(right.x + g_impactJitterX),
    right.width,
    18
  );
  left.y = (int16_t)(left.y + g_impactJitterY);
  right.y = (int16_t)(right.y + g_impactJitterY);
}

void triggerEyeImpact(uint32_t now) {
  g_impactActive = true;
  g_impactStartMs = now;
  g_impactDurationMs = anim::randRangeMs(180, 260);
  g_impactJitterX = (int8_t)(anim::randRangeMs(0, 4) - 2);
  g_impactJitterY = (int8_t)(anim::randRangeMs(0, 4) - 2);
  eyes::requestForceRedraw();
}
