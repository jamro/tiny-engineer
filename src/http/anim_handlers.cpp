#include <Arduino.h>

#include "animation.h"
#include "api/commands.h"
#include "http/handlers.h"
#include "http/response.h"

void handleAnimGet(WebServer& server) {
  AnimationId id = AnimationId::None;
  apiGetAnimation(id);

  JsonDocument body;
  body["ok"] = true;
  body["animation"] = animationName(id);
  sendJson(server, 200, body);
}

void handleAnimPost(WebServer& server) {
  if (!server.hasArg("name")) {
    sendError(server, 400, "missing name");
    return;
  }

  const ApiResult result = apiSetAnimation(server.arg("name").c_str());
  if (!result.ok) {
    sendError(server, result.code, result.error);
    return;
  }

  handleAnimGet(server);
}
