#include "http/play_handlers.h"

#include <Arduino.h>

#include "audio/play_session.h"
#include "http/json.h"

namespace {

constexpr size_t kPlayPrefixLen = sizeof("/play/") - 1;

PlaySession g_session;

}  // namespace

void handlePlayBody(WebServer& server) {
  HTTPRaw& raw = server.raw();

  switch (raw.status) {
    case RAW_START: {
      g_session.clear();
      if (!httpApiAuthorized(server)) {
        g_session.reject(401, "unauthorized");
        break;
      }
      const String uri = server.uri();
      const char* name =
          uri.length() > kPlayPrefixLen ? uri.c_str() + kPlayPrefixLen : "talking";
      g_session.begin(name);
      break;
    }
    case RAW_WRITE:
      if (!g_session.rejected()) {
        g_session.feed(raw.buf, raw.currentSize);
      }
      break;
    case RAW_END:
    case RAW_ABORTED:
      g_session.end();
      break;
  }
}

void handlePlayDone(WebServer& server) {
  httpSendJson(server, g_session.statusCode(), g_session.bodyJson());
}
