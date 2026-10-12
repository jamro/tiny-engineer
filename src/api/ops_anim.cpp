#include "api/ops.h"

#include <cstdio>

#include "animation.h"

namespace {

void sendAnimationJson(ApiResponse& resp) {
  char body[64];
  std::snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"animation\":\"%s\"}",
    animationName(getAnimation())
  );
  resp.set(200, body);
}

}  // namespace

void apiOpAnimGet(const ApiRequest& /*req*/, ApiResponse& resp) {
  sendAnimationJson(resp);
}

void apiOpAnimPost(const ApiRequest& req, ApiResponse& resp) {
  if (!req.hasArg("name")) {
    resp.setError(400, "missing name");
    return;
  }

  AnimationId id;
  if (!parseAnimationName(req.arg("name"), id)) {
    resp.setError(400, "unknown animation");
    return;
  }

  setAnimation(id);
  sendAnimationJson(resp);
}
