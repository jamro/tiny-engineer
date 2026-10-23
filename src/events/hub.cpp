#include "events/hub.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <cstring>

#include "serial/link.h"
#include "serial_log.h"
#include "settings/settings.h"

namespace {

constexpr uint16_t kEventsPort = 81;
constexpr size_t kMaxClients = 4;

WebSocketsServer g_ws(kEventsPort);
bool g_started = false;

bool pathIsEvents(const char* url) {
  if (url == nullptr || url[0] == '\0') {
    return false;
  }

  // "/events" or "/events?token=..."
  if (strncmp(url, "/events", 7) != 0) {
    return false;
  }

  return url[7] == '\0' || url[7] == '?' || url[7] == '#';
}

bool tokenOk(const char* url) {
  if (!settingsAccessTokenSet()) {
    return true;
  }

  if (url == nullptr) {
    return false;
  }

  const char* query = strchr(url, '?');
  if (query == nullptr) {
    return false;
  }

  const char* token = settingsAccessToken();
  const size_t tokenLen = strlen(token);

  for (const char* p = query + 1; *p != '\0';) {
    const char* amp = strchr(p, '&');
    const size_t partLen = amp != nullptr ? static_cast<size_t>(amp - p) : strlen(p);

    if (partLen > 6 && strncmp(p, "token=", 6) == 0) {
      const char* value = p + 6;
      const size_t valueLen = partLen - 6;
      if (valueLen == tokenLen && memcmp(value, token, tokenLen) == 0) {
        return true;
      }
      return false;
    }

    if (amp == nullptr) {
      break;
    }
    p = amp + 1;
  }

  return false;
}

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  (void)length;

  switch (type) {
    case WStype_CONNECTED: {
      const char* url = reinterpret_cast<const char*>(payload);
      if (!pathIsEvents(url) || !tokenOk(url)) {
        g_ws.disconnect(num);
        return;
      }
      break;
    }
    case WStype_TEXT:
    case WStype_BIN:
      // Outbound only — ignore client payloads.
      break;
    default:
      break;
  }
}

}  // namespace

void eventsHubBegin() {
  if (g_started) {
    return;
  }

  g_ws.begin();
  g_ws.onEvent(onWsEvent);
  g_started = true;
  serialLogPrintln("Events: ws://*:81/events");
}

void eventsHubEnd() {
  if (!g_started) {
    return;
  }

  g_ws.close();
  g_started = false;
  serialLogPrintln("Events: stopped");
}

void eventsHubPoll() {
  if (!g_started) {
    return;
  }

  g_ws.loop();
}

void eventsHubDisconnectAll() {
  if (!g_started) {
    return;
  }

  g_ws.disconnect();
}

void publishEvent(const char* type, const char* source) {
  if (type == nullptr || source == nullptr) {
    return;
  }

  JsonDocument doc;
  doc["v"] = 1;
  doc["type"] = type;
  doc["source"] = source;
  doc["ms"] = millis();

  char json[192];
  const size_t len = serializeJson(doc, json, sizeof(json));
  if (len == 0 || len >= sizeof(json)) {
    return;
  }

  if (g_started) {
    g_ws.broadcastTXT(json, len);
  }

  serialLinkEmitEvent(json, len);
}

void publishBinary(const uint8_t* data, size_t length) {
  (void)data;
  (void)length;
}
