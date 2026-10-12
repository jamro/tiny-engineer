#include "api/ops.h"

#include <cstdio>

#include "animation.h"
#include "hardware/pca9685_servos.h"
#include "hardware/servo_wrapper.h"

void apiOpServosPark(const ApiRequest& /*req*/, ApiResponse& resp) {
  setAnimationImmediately(AnimationId::None);
  ensureAllServoOutputs();
  centerAllServos();
  flushServoPoseToNvs();
  setServoParkHoldUntilReboot(true);

  resp.set(200, "{\"ok\":true,\"park\":\"servos\",\"hold\":true}");
}
