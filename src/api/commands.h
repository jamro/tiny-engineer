#pragma once

#include "animation.h"
#include "settings/settings.h"

// Transport-agnostic command results. HTTP and serial adapters map these to their wire format.

struct ApiResult {
  bool ok = false;
  int code = 500;
  const char* error = nullptr;
};

ApiResult apiSetAnimation(const char* name);
ApiResult apiGetAnimation(AnimationId& out);
ApiResult apiFactoryReset();
ApiResult apiSaveSettings(const SettingsPatch& patch);

// name: audio | bell | screen | movement | led
ApiResult apiRunHardwareTest(const char* name);

// Joint index 0..SETTINGS_SERVO_COUNT-1, angle in degrees.
ApiResult apiMoveServo(int index, float angle);
