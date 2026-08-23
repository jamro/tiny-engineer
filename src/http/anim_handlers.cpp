#include "http/anim_handlers.h"

#include <Arduino.h>
#include <cstdio>

#include "animation.h"
#include "http/json.h"

namespace {

void sendAnimationJson(WebServer& server) {
  char body[64];

  snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"animation\":\"%s\"}",
    animationName(getAnimation())
  );

  httpSendJson(server, 200, body);
}

}  // namespace

void handleAnimGet(WebServer& server) {
  sendAnimationJson(server);
}

void handleAnimPost(WebServer& server) {
  if (!server.hasArg("name")) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"missing name\"}"
    );
    return;
  }

  AnimationId id;

  if (!parseAnimationName(server.arg("name").c_str(), id)) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"unknown animation\"}"
    );
    return;
  }

  setAnimation(id);
  sendAnimationJson(server);
}
