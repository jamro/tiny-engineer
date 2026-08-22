#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>

#include "pins.h"
#include "rgb.h"
#include "oled.h"
#include "audio.h"
#include "animation.h"
#include "pca9685_servos.h"
#include "wifi_connect.h"
#include "http_server.h"
#include "sleep.h"

namespace {

constexpr int kBootSteps = 6;

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==========================");
  Serial.println("TINY ENGINEER");
  Serial.println("==========================");

  setRgb(0, 32, 0);

  Serial.println();
  Serial.println("Starting I2C");
  Serial.println("SDA = GP0");
  Serial.println("SCL = GP1");

  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );

  delay(100);

  initOled();
  showBootProgress(1, kBootSteps, "Display");

  showBootProgress(2, kBootSteps, "WiFi...");
  runWifiTest();
  showBootProgress(
    2,
    kBootSteps,
    wifiConnected() ? "WiFi OK" : "WiFi failed"
  );

  initPca9685();
  showBootProgress(3, kBootSteps, "Servos");

  Serial.println();
  Serial.println(
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
    Serial.println(
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

  Serial.println("I2S OK");
  showBootProgress(4, kBootSteps, "Audio");

  initAudioStorage();
  showBootProgress(5, kBootSteps, "Storage");

  Serial.println("Centering servos");
  centerAllServos();
  showBootProgress(kBootSteps, kBootSteps, "Ready");

  Serial.println();
  Serial.println("==========================");
  Serial.println("ROBOT READY");
  Serial.println("==========================");

  showIdleScreen();

  if (wifiConnected()) {
    setAnimation(AnimationId::Welcome);
  } else {
    setRgbForAnimation(AnimationId::None, millis());
  }

  startHttpServer();
  initSleep();
}

void loop() {
  const uint32_t now = millis();
  pollHttpServer();
  updateSleep(now);
  updateAnimation();
  updateRgb(now);
}
