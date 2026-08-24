#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>

#include "pins.h"
#include "hardware/rgb.h"
#include "hardware/chip_temp.h"
#include "display/oled.h"
#include "audio/audio.h"
#include "animation.h"
#include "hardware/pca9685_servos.h"
#include "hardware/servo_wrapper.h"
#include "network/wifi_connect.h"
#include "http/http_server.h"
#include "settings.h"
#include "serial_log.h"
#include "sleep.h"
#include "boot/boot_loading.h"

namespace {

constexpr int kBootSteps = 6;

}  // namespace

void setup() {
  initServoOutputPin();

  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  delay(1000);

  initSettings();

  initChipTemp();

  serialLogPrintln();
  serialLogPrintln("==========================");
  serialLogPrintln("TINY ENGINEER");
  serialLogPrintln("==========================");

  setRgb(0, 32, 0);

  serialLogPrintln();
  serialLogPrintln("Starting I2C");
  serialLogPrintln("SDA = GP0");
  serialLogPrintln("SCL = GP1");

  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );

  delay(100);

  initPca9685();

  initOled();

  const bool sleepServos = bootSleepInertiaUsesServos();

  if (bootLoadingIsProgress()) {
    bootShowProgress(1, kBootSteps, "Display");
  } else {
    bootBeginSleepingFace();
  }

  if (sleepServos) {
    bootSnapSleepPose();
  }

  bootShowProgress(2, kBootSteps, "WiFi...");
  runWifiSetup();
  bootShowProgress(
    2,
    kBootSteps,
    wifiConnected() ? "WiFi OK" :
    wifiProvisioningMode() ? "WiFi setup" : "WiFi failed"
  );

  bootShowProgress(3, kBootSteps, "Servos");

  serialLogPrintln();
  serialLogPrintln(
    "Starting MAX98357A"
  );

  I2S.setPins(
    I2S_BCLK,
    I2S_LRC,
    I2S_DIN
  );

  if (!I2S.begin(
        I2S_MODE_STD,
        SAMPLE_RATE,
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_STEREO
      )) {
    serialLogPrintln(
      "ERROR: I2S initialization failed"
    );

    showOledText(
      "I2S ERROR",
      "Init failed"
    );

    setRgb(64, 0, 0);

    while (true) {
      delay(1000);
    }
  }

  serialLogPrintln("I2S OK");
  bootShowProgress(4, kBootSteps, "Audio");

  initAudioStorage();
  bootShowProgress(5, kBootSteps, "Storage");

  if (!sleepServos) {
    serialLogPrintln("Centering servos");
    centerAllServos();
  }

  if (bootLoadingIsProgress()) {
    bootShowProgress(kBootSteps, kBootSteps, "Ready");
    if (wifiProvisioningMode()) {
      showProvisioningOled("Join this WiFi", wifiApSsid());
    } else {
      showBootIp(wifiConnected() ? wifiIpText() : "");
      delay(3000);
      showIdleScreen();
    }
  } else {
    bootRunSleepInertia();
    if (wifiProvisioningMode()) {
      showProvisioningOled("Join this WiFi", wifiApSsid());
    }
  }

  serialLogPrintln();
  serialLogPrintln("==========================");
  serialLogPrintln("ROBOT READY");
  serialLogPrintln("==========================");

  if (wifiConnected() && settingsWelcomeEnabled()) {
    setAnimation(AnimationId::Welcome);
  } else if (wifiProvisioningMode()) {
    setRgb(0, 0, 64);
  } else {
    setRgbForAnimation(AnimationId::None, millis());
  }

  if (wifiConnected() || wifiProvisioningMode()) {
    startHttpServer();
  }
  initSleep();
}

void loop() {
  const uint32_t now = millis();
  pollWifi();
  pollHttpServer();
  updateProvisioningOled(now);
  updateAnimation();
  // After HTTP/anim so idle timestamp from this frame is not compared to a stale `now`.
  updateSleep(millis());
  updateRgb(now);
}
