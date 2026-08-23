#include "http/server_context.h"

#include <cstdio>

#include "settings.h"

namespace {

WebServer server(80);
char mdnsHostname[SETTINGS_HOSTNAME_MAX_LEN + 7];

}  // namespace

WebServer& httpServer() {
  return server;
}

void refreshMdnsHostname() {
  snprintf(
    mdnsHostname,
    sizeof(mdnsHostname),
    "%s.local",
    settingsHostname()
  );
}

const char* httpMdnsHostname() {
  return mdnsHostname;
}
