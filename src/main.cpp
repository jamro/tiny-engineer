#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>

#include "pins.h"
#include "rgb.h"
#include "oled.h"
#include "audio.h"
#include "pca9685_servos.h"
#include "wifi_connect.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==========================");
  Serial.println("ROBOT HARDWARE TEST");
  Serial.println("==========================");

  runRgbTest();

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
  runOledTest();

  runWifiTest();

  showOledText(
    "PCA9685",
    "Checking..."
  );

  initPca9685();

  showOledText(
    "PCA9685",
    "OK"
  );

  delay(500);

  Serial.println();
  Serial.println(
    "Starting MAX98357A"
  );

  showOledText(
    "MAX98357A",
    "Starting..."
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

  showOledText(
    "MAX98357A",
    "I2S OK"
  );

  delay(500);

  runSoundTest();

  delay(500);

  runServoTest();

  Serial.println();
  Serial.println("==========================");
  Serial.println("ALL TESTS FINISHED");
  Serial.println("==========================");

  setRgb(
    0,
    32,
    0
  );

  showOledText(
    "ROBOT READY",
    wifiConnected() ? wifiIpText() : "WIFI FAIL"
  );
}

void loop() {
  delay(1000);
}
