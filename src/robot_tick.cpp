#include "robot_tick.h"

#include <Arduino.h>

#include "animation.h"
#include "audio/wav_stream.h"
#include "display/oled.h"
#include "hardware/rgb.h"
#include "hardware/servo_wrapper.h"
#include "network/wifi_connect.h"
#include "sleep.h"

namespace {

bool animationDrivesWavStream(AnimationId id) {
  switch (id) {
    case AnimationId::Ring:
    case AnimationId::Welcome:
    case AnimationId::Attention:
    case AnimationId::Error:
    case AnimationId::Abort:
    case AnimationId::Dead:
      return true;
    default:
      return false;
  }
}

}  // namespace

void tickRobot() {
  const uint32_t now = millis();
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
}
