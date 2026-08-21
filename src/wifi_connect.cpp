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
constexpr unsigned long IPV6_TIMEOUT_MS = 2000;

void waitForLinkLocalIpv6() {
  const unsigned long startMs = millis();

  while (
      !WiFi.STA.hasLinkLocalIPv6() &&
      millis() - startMs < IPV6_TIMEOUT_MS
  ) {
    delay(50);
  }

  if (WiFi.STA.hasLinkLocalIPv6()) {
    Serial.print("IPv6 LL: ");
    Serial.println(WiFi.linkLocalIPv6());
  } else {
    Serial.println("IPv6 LL: timeout");
  }
}

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
  // macOS getaddrinfo waits 2–3s on unanswered AAAA for *.local.
  // Publish link-local IPv6 so mDNS answers AAAA immediately.
  WiFi.enableIPv6(true);
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

    // Modem sleep drops mDNS multicast. AP buffers unicast (raw IP)
    // but not mDNS, so .local waits 2–3s for a retry. USB desk robot:
    // keep the radio awake.
    WiFi.setSleep(false);

    Serial.print("WIFI OK  IP=");
    Serial.println(ipText);
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());

    waitForLinkLocalIpv6();

    if (MDNS.begin(WIFI_HOSTNAME)) {
      MDNS.addService("http", "tcp", 80);
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
