#include "robot_tick.h"

#include <Arduino.h>

#include "animation.h"
#include "audio/wav_stream.h"
#include "display/oled.h"
#include "extensions/lcd1602/lcd1602.h"
#include "hardware/i2c_health.h"
#include "hardware/rgb.h"
#include "hardware/servo_wrapper.h"
#include "network/wifi_connect.h"
#include "sleep.h"

void tickRobot() {
  const uint32_t now = millis();
  updateI2cHealth();
  pollWifi();
  updateProvisioningOled(now);
  updateAnimation();
  // Button SFX (and any other orphan LittleFS stream) needs a pump when no
  // animation owns the stream; anim updates already call wavStreamUpdate.
  if (wavStreamIsActive() && !animationDrivesWavStream(getAnimation())) {
    wavStreamUpdate();
  }
  // Read again: an animation applied this tick stamps its idle time after `now`.
  updateSleep(millis());
  updateRgb(now);
  pollServoPoseSave();
  // Also needed from PlaySession::onPcm / serial play — main loop is blocked
  // for the whole streamed clip, so marquee + backlight fades stall otherwise.
  lcd1602Poll();
}
