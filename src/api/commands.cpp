#include "api/commands.h"

#include <Arduino.h>
#include <cstring>

#include "animation.h"
#include "audio/audio.h"
#include "display/oled.h"
#include "extensions/push_button.h"
#include "hardware/pca9685_servos.h"
#include "hardware/rgb.h"
#include "hardware/servo_wrapper.h"
#include "network/wifi_connect.h"
#include "servos.h"
#include "settings/settings.h"

ApiResult apiSetAnimation(const char* name) {
  if (name == nullptr) {
    return {false, 400, "missing name"};
  }

  AnimationId id;
  if (!parseAnimationName(name, id)) {
    return {false, 400, "unknown animation"};
  }

  setAnimation(id);
  return {true, 200, nullptr};
}

ApiResult apiGetAnimation(AnimationId& out) {
  out = getAnimation();
  return {true, 200, nullptr};
}

ApiResult apiFactoryReset() {
  if (!factoryResetSettings()) {
    return {false, 500, "factory reset failed"};
  }
  pushButtonApply(settingsPushButton());
  return {true, 200, nullptr};
}

ApiResult apiSaveSettings(const SettingsPatch& patch) {
  if (patch.invalidField() != nullptr) {
    return {false, 400, "invalid settings"};
  }
  if (const char* linkErr = settingsLinkModePatchDenied(
        patch.linkMode,
        settingsLinkMode(),
        settingsProvisioned(),
        wifiProvisioningMode()
      );
      linkErr != nullptr) {
    return {false, 400, linkErr};
  }
  if (!saveSettings(patch)) {
    return {false, 500, "save failed"};
  }
  if (patch.pushButton.has_value()) {
    pushButtonApply(settingsPushButton());
  }
  return {true, 200, nullptr};
}

ApiResult apiRunHardwareTest(const char* name) {
  if (name == nullptr) {
    return {false, 400, "missing test"};
  }

  if (strcmp(name, "audio") == 0) {
    runSoundTest();
    showIdleScreen();
    return {true, 200, nullptr};
  }
  if (strcmp(name, "bell") == 0) {
    if (!playBell()) {
      showIdleScreen();
      return {false, 500, "bell playback failed"};
    }
    showIdleScreen();
    return {true, 200, nullptr};
  }
  if (strcmp(name, "screen") == 0) {
    runOledTest();
    showIdleScreen();
    return {true, 200, nullptr};
  }
  if (strcmp(name, "movement") == 0) {
    runServoTest();
    showIdleScreen();
    return {true, 200, nullptr};
  }
  if (strcmp(name, "led") == 0) {
    runRgbTest();
    setRgbForAnimation(getAnimation(), millis());
    showIdleScreen();
    return {true, 200, nullptr};
  }

  return {false, 400, "unknown test"};
}

ApiResult apiMoveServo(int index, float angle) {
  if (index < 0 || index >= static_cast<int>(SERVO_COUNT)) {
    return {false, 400, "invalid servo index"};
  }
  if (angle < 0.0f || angle > static_cast<float>(SETTINGS_SERVO_ANGLE_MAX)) {
    return {false, 400, "invalid servo angle"};
  }

  moveServoSmooth(index, angle);
  showIdleScreen();
  return {true, 200, nullptr};
}

ApiResult apiParkServos() {
  setAnimationImmediately(AnimationId::None);
  ensureAllServoOutputs();
  centerAllServos();
  const bool poseSaved = flushServoPoseToNvs();
  setServoParkHoldUntilReboot(true);

  if (!poseSaved) {
    return {false, 400, "pose save failed"};
  }

  return {true, 200, nullptr};
}
