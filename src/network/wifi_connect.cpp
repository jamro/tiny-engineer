#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

#include <algorithm>
#include <cstdint>

#include "settings/settings.h"
#include "settings/wifi_network_list.h"
#include "network/wifi_connect.h"
#include "serial_log.h"

namespace {

constexpr unsigned long WIFI_TIMEOUT_MS = 20000;
// Long enough for the core's retries after "AP not found", which a hidden
// SSID can need, but short enough that out-of-range networks do not stall
// boot for the full timeout each.
constexpr unsigned long WIFI_UNSEEN_TIMEOUT_MS = 8000;
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

// Phones open the setup page on their own: DHCP option 114 names the portal
// URL, and the catch-all DNS sends connectivity probes to the web server.
void openSetupAp(wifi_mode_t mode) {
  buildApSsid();

  WiFi.persistent(false);
  WiFi.mode(mode);
  WiFi.softAP(apSsid);

  if (!WiFi.AP.enableDhcpCaptivePortal()) {
    serialLogPrintln("ERROR: DHCP captive portal option failed");
  }

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

bool connectSta(
  const char* ssid,
  const char* password,
  unsigned long timeoutMs = WIFI_TIMEOUT_MS
) {
  const char* hostname = settingsHostname();

  serialLogPrint("SSID: ");
  serialLogPrintln(ssid);
  serialLogPrint("Hostname: ");
  serialLogPrintln(hostname);

  WiFi.persistent(false);
  WiFi.disconnect(false);
  WiFi.setHostname(hostname);
  WiFi.enableIPv6(true);
  WiFi.begin(ssid, password);

  const unsigned long startMs = millis();

  while (
      WiFi.status() != WL_CONNECTED &&
      millis() - startMs < timeoutMs
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

// Strongest visible network first. Networks the scan missed keep their saved
// order at the end, since a hidden SSID never shows up in a scan.
bool connectSavedNetworks() {
  const WifiNetworkList& networks = settingsWifiNetworks();
  // A single network is tried with the full timeout, so it needs no scan.
  // A failed scan (negative result) also gives every network the full timeout.
  const int found = networks.count() > 1 ? WiFi.scanNetworks() : -1;
  const bool scanned = found >= 0;
  int32_t rssi[SETTINGS_WIFI_NETWORK_MAX];
  size_t order[SETTINGS_WIFI_NETWORK_MAX];

  std::fill(rssi, rssi + networks.count(), INT32_MIN);

  for (int i = 0; i < found; i++) {
    const int index = networks.find(WiFi.SSID(i).c_str());

    if (index >= 0) {
      rssi[index] = std::max(rssi[index], WiFi.RSSI(i));
    }
  }

  WiFi.scanDelete();

  for (size_t i = 0; i < networks.count(); i++) {
    order[i] = i;
  }

  std::stable_sort(
    order,
    order + networks.count(),
    [&rssi](size_t a, size_t b) { return rssi[a] > rssi[b]; }
  );

  return std::any_of(
    order,
    order + networks.count(),
    [&](size_t index) {
      const WifiNetwork& network = networks.at(index);
      const bool unseen = scanned && rssi[index] == INT32_MIN;

      return connectSta(
        network.ssid,
        network.password,
        unseen ? WIFI_UNSEEN_TIMEOUT_MS : WIFI_TIMEOUT_MS
      );
    }
  );
}

void startProvisioningAp() {
  openSetupAp(WIFI_AP);

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

String wifiMdnsHostname() {
  return String(settingsHostname()) + ".local";
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

void wifiRestoreProvisioningAp() {
  WiFi.disconnect(false);
  startProvisioningAp();
}

bool wifiTestCredentials(const char* ssid, const char* password) {
  serialLogPrintln();
  serialLogPrintln("==========================");
  serialLogPrintln("WIFI TEST (provisioning)");
  serialLogPrintln("==========================");

  openSetupAp(WIFI_AP_STA);

  if (!connectSta(ssid, password)) {
    wifiRestoreProvisioningAp();
    return false;
  }

  return true;
}

void runWifiSetup() {
  serialLogPrintln();
  serialLogPrintln("==========================");
  serialLogPrintln("WIFI SETUP");
  serialLogPrintln("==========================");

  connectError[0] = '\0';

  if (strcmp(settingsLinkMode(), "usb") == 0) {
    serialLogPrintln("USB-only mode; skipping WiFi and HTTP");
    provisioning = false;
    return;
  }

  if (settingsWifiConfigured()) {
    serialLogPrintln("Trying saved WiFi credentials");

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    if (connectSavedNetworks()) {
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
