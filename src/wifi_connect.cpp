#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy include/secrets.h.example to include/secrets.h and set WIFI_SSID / WIFI_PASSWORD"
#endif

#include "oled.h"
#include "wifi_connect.h"

namespace {

constexpr const char* WIFI_HOSTNAME = "tiny-engineer";
constexpr unsigned long WIFI_TIMEOUT_MS = 20000;
constexpr unsigned long WIFI_POLL_MS = 500;

char ipText[16] = "";
bool connected = false;

const char* failReason(wl_status_t status) {
  switch (status) {
    case WL_NO_SSID_AVAIL:
      return "SSID not found";
    case WL_CONNECT_FAILED:
      return "Auth failed";
    case WL_CONNECTION_LOST:
      return "Lost";
    default:
      return "Timeout";
  }
}

void storeIp(const IPAddress& ip) {
  snprintf(
    ipText,
    sizeof(ipText),
    "%u.%u.%u.%u",
    ip[0],
    ip[1],
    ip[2],
    ip[3]
  );
}

}  // namespace

bool wifiConnected() {
  return connected;
}

const char* wifiIpText() {
  return ipText;
}

void runWifiTest() {
  Serial.println();
  Serial.println("==========================");
  Serial.println("WIFI TEST");
  Serial.println("==========================");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("Hostname: ");
  Serial.println(WIFI_HOSTNAME);

  showOledText(
    "WIFI",
    "Connecting..."
  );

  // Hostname must be set before WiFi.mode() / WiFi.begin()
  // or the DHCP client keeps the default esp32c3-XXXX name.
  WiFi.persistent(false);
  WiFi.disconnect(true);
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  const unsigned long startMs = millis();

  while (
      WiFi.status() != WL_CONNECTED &&
      millis() - startMs < WIFI_TIMEOUT_MS
  ) {
    delay(WIFI_POLL_MS);
    Serial.print(".");
  }

  Serial.println();

  connected = WiFi.status() == WL_CONNECTED;

  if (connected) {
    storeIp(WiFi.localIP());

    Serial.print("WIFI OK  IP=");
    Serial.println(ipText);
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());

    if (MDNS.begin(WIFI_HOSTNAME)) {
      Serial.println("mDNS: tiny-engineer.local");
    } else {
      Serial.println("ERROR: mDNS failed");
    }

    showOledText(
      "WIFI OK",
      ipText
    );
  } else {
    const char* reason = failReason(WiFi.status());

    Serial.print("ERROR: WIFI failed: ");
    Serial.println(reason);

    showOledText(
      "WIFI FAIL",
      reason
    );
  }

  delay(1500);
}
