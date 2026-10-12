#include "http/health_handlers.h"

#include "api/http_bridge.h"
#include "http/server_context.h"

void handleAuth() {
  httpDispatchApi(httpServer());
}

void handleHealth(WebServer& server) {
  httpDispatchApi(server);
}
