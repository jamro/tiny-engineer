#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

#include "settings.h"
#include "network/wifi_connect.h"
#include "serial_log.h"

namespace {

constexpr unsigned long WIFI_TIMEOUT_MS = 20000;
constexpr unsigned long WIFI_POLL_MS = 500;
constexpr unsigned long IPV6_TIMEOUT_MS = 2000;
constexpr size_t AP_SSID_MAX = 20;

DNSServer dnsServer;
bool dnsStarted = false;

char ipText[16] = "";
char apSsid[AP_SSID_MAX + 1] = "";
char apIpText[16] = "192.168.4.1";
char connectError[32] = "";

bool staConnected = false;
bool provisioning = false;

void setConnectError(const char* msg) {
  strncpy(connectError, msg, sizeof(connectError) - 1);
  connectError[sizeof(connectError) - 1] = '\0';
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

void buildApSsid() {
  const uint64_t chipId = ESP.getEfuseMac();
  const uint16_t suffix =
    static_cast<uint16_t>((chipId >> 8) & 0xFFFF);

  snprintf(
    apSsid,
    sizeof(apSsid),
    "TinyEngineer-%04X",
    suffix
  );
}

void waitForLinkLocalIpv6() {
  const unsigned long startMs = millis();

  while (
      !WiFi.STA.hasLinkLocalIPv6() &&
      millis() - startMs < IPV6_TIMEOUT_MS
  ) {
    delay(50);
  }

  if (WiFi.STA.hasLinkLocalIPv6()) {
    serialLogPrint("IPv6 LL: ");
    serialLogPrintln(WiFi.linkLocalIPv6());
  } else {
    serialLogPrintln("IPv6 LL: timeout");
  }
}

void startMdns(const char* hostname) {
  if (MDNS.begin(hostname)) {
    MDNS.addService("http", "tcp", 80);
    serialLogPrint("mDNS: ");
    serialLogPrint(hostname);
    serialLogPrintln(".local");
  } else {
    serialLogPrintln("ERROR: mDNS failed");
  }
}

void startCaptiveDns() {
  if (dnsStarted) {
    return;
  }

  dnsServer.start(53, "*", WiFi.softAPIP());
  dnsStarted = true;
  serialLogPrintln("DNS: captive portal active");
}

void stopCaptiveDns() {
  if (!dnsStarted) {
    return;
  }

  dnsServer.stop();
  dnsStarted = false;
}

bool connectSta(const char* ssid, const char* password) {
  const char* hostname = settingsHostname();

  serialLogPrint("SSID: ");
  serialLogPrintln(ssid);
  serialLogPrint("Hostname: ");
  serialLogPrintln(hostname);

  WiFi.persistent(false);
  WiFi.disconnect(true);
  WiFi.setHostname(hostname);
  WiFi.enableIPv6(true);
  WiFi.begin(ssid, password);

  const unsigned long startMs = millis();

  while (
      WiFi.status() != WL_CONNECTED &&
      millis() - startMs < WIFI_TIMEOUT_MS
  ) {
    delay(WIFI_POLL_MS);
    serialLogPrint(".");
  }

  serialLogPrintln();

  if (WiFi.status() != WL_CONNECTED) {
    setConnectError(failReason(WiFi.status()));
    staConnected = false;
    ipText[0] = '\0';

    serialLogPrint("ERROR: WIFI failed: ");
    serialLogPrintln(connectError);
    return false;
  }

  storeIp(WiFi.localIP());
  staConnected = true;
  WiFi.setSleep(false);

  serialLogPrint("WIFI OK  IP=");
  serialLogPrintln(ipText);
  serialLogPrint("Hostname: ");
  serialLogPrintln(WiFi.getHostname());

  waitForLinkLocalIpv6();
  startMdns(hostname);
  return true;
}

void startProvisioningAp() {
  buildApSsid();

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid);

  const IPAddress apIp = WiFi.softAPIP();
  storeIp(apIp);
  snprintf(
    apIpText,
    sizeof(apIpText),
    "%u.%u.%u.%u",
    apIp[0],
    apIp[1],
    apIp[2],
    apIp[3]
  );

  provisioning = true;
  staConnected = false;
  ipText[0] = '\0';

  startCaptiveDns();

  serialLogPrint("AP setup SSID: ");
  serialLogPrintln(apSsid);
  serialLogPrint("AP IP: ");
  serialLogPrintln(apIpText);
}

}  // namespace

bool wifiConnected() {
  return staConnected;
}

bool wifiProvisioningMode() {
  return provisioning;
}

const char* wifiIpText() {
  return ipText;
}

const char* wifiApSsid() {
  return apSsid;
}

const char* wifiApIpText() {
  return apIpText;
}

const char* wifiLastConnectError() {
  return connectError;
}

void pollWifi() {
  if (dnsStarted) {
    dnsServer.processNextRequest();
  }
}

void wifiStopProvisioningAp() {
  stopCaptiveDns();
  WiFi.softAPdisconnect(true);
  provisioning = false;
}

bool wifiTestCredentials(const char* ssid, const char* password) {
  if (ssid == nullptr || password == nullptr) {
    setConnectError("Missing credentials");
    return false;
  }

  if (!settingsValidateWifiSsid(ssid)) {
    setConnectError("Invalid SSID");
    return false;
  }

  if (!settingsValidateWifiPassword(password)) {
    setConnectError("Password too long");
    return false;
  }

  serialLogPrintln();
  serialLogPrintln("==========================");
  serialLogPrintln("WIFI TEST (provisioning)");
  serialLogPrintln("==========================");

  buildApSsid();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSsid);
  startCaptiveDns();

  if (!connectSta(ssid, password)) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid);
    provisioning = true;
    staConnected = false;
    ipText[0] = '\0';
    return false;
  }

  stopCaptiveDns();
  WiFi.softAPdisconnect(true);
  provisioning = false;
  return true;
}

void runWifiSetup() {
  serialLogPrintln();
  serialLogPrintln("==========================");
  serialLogPrintln("WIFI SETUP");
  serialLogPrintln("==========================");

  connectError[0] = '\0';

  if (settingsWifiConfigured()) {
    serialLogPrintln("Trying saved WiFi credentials");

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    if (connectSta(settingsWifiSsid(), settingsWifiPassword())) {
      provisioning = false;
      delay(500);
      return;
    }

    serialLogPrintln("Saved WiFi failed; opening setup AP");
  } else {
    serialLogPrintln("No WiFi credentials; opening setup AP");
  }

  startProvisioningAp();
  delay(500);
}
