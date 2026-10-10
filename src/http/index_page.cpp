#include "http/index_page.h"

#include "web_ui.h"

void sendIndexPage(WebServer& server) {
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html; charset=utf-8", reinterpret_cast<const char*>(kWebUiGzip), sizeof(kWebUiGzip));
}
