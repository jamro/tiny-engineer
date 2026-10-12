#include "http/anim_handlers.h"

#include "api/http_bridge.h"

void handleAnimGet(WebServer& server) {
  httpDispatchApi(server);
}

void handleAnimPost(WebServer& server) {
  httpDispatchApi(server);
}
