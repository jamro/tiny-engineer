#include <Arduino.h>

#include "api/commands.h"
#include "display/oled.h"
#include "http/handlers.h"
#include "http/request.h"
#include "http/response.h"

namespace {

JsonDocument testReply(const char* test) {
  JsonDocument reply;
  reply["ok"] = true;
  reply["test"] = test;
  return reply;
}

void finishTest(WebServer& server, const JsonDocument& reply) {
  showIdleScreen();
  sendJson(server, 200, reply);
}

void runNamedTest(WebServer& server, const char* name) {
  const ApiResult result = apiRunHardwareTest(name);
  if (!result.ok) {
    showIdleScreen();
    sendError(server, result.code, result.error);
    return;
  }
  finishTest(server, testReply(name));
}

}  // namespace

void handleAudioTest(WebServer& server) {
  runNamedTest(server, "audio");
}

void handleBellTest(WebServer& server) {
  runNamedTest(server, "bell");
}

void handleScreenTest(WebServer& server) {
  runNamedTest(server, "screen");
}

void handleMovementTest(WebServer& server) {
  runNamedTest(server, "movement");
}

void handleLedTest(WebServer& server) {
  runNamedTest(server, "led");
}

void handleServoTest(WebServer& server) {
  JsonDocument request;

  if (!readJsonBody(server, request)) {
    return;
  }

  const std::optional<ServoMove> move = ServoMove::read(server, request.as<JsonObjectConst>());

  if (!move) {
    return;
  }

  const ApiResult result = apiMoveServo(move->index, move->angle);
  if (!result.ok) {
    showIdleScreen();
    sendError(server, result.code, result.error);
    return;
  }

  JsonDocument reply = testReply("servo");
  move->addTo(reply);
  finishTest(server, reply);
}
