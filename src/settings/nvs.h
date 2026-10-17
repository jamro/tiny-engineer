#pragma once

#include <Arduino.h>
#include <Preferences.h>

constexpr const char* kNs = "te";
constexpr const char* kKeySleep = "sleep_m";
constexpr const char* kKeyHost = "host";
constexpr const char* kKeyVolume = "vol";
constexpr const char* kKeyWelcome = "welcome";
constexpr const char* kKeySerialLog = "serial_log";
constexpr const char* kKeyContTo = "cont_to";
constexpr const char* kKeyLoading = "loading";
constexpr const char* kKeyEyesStyle = "eyes_sty";
constexpr const char* kKeyAccessTok = "access_tok";
// Single-network keys from older firmware; moved into kKeyWifiNets at boot.
constexpr const char* kKeyWifiSsid = "wifi_ssid";
constexpr const char* kKeyWifiPass = "wifi_pass";
constexpr const char* kKeyWifiNets = "wifi_nets";
constexpr const char* kKeyServoRanges = "sranges";
constexpr const char* kKeyRgbOrder = "rgb_ord";
constexpr const char* kKeyOledRot = "oled_rot";
constexpr const char* kKeyLinkMode = "link_mode";

extern Preferences g_settingsPrefs;
