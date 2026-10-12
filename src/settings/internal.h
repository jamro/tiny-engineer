#pragma once

#include <cstdint>

#include "settings/settings.h"
#include "settings/wifi_network_list.h"

extern uint32_t g_sleepTimeoutMin;
extern char g_hostname[SETTINGS_HOSTNAME_MAX_LEN + 1];
extern char g_bootHostname[SETTINGS_HOSTNAME_MAX_LEN + 1];
extern uint8_t g_volume;
extern bool g_welcome;
extern bool g_serialLog;
extern bool g_servoHold;
extern bool g_wifiEnabled;
extern bool g_bootWifiEnabled;
extern uint32_t g_continuousTimeoutMin;
extern char g_loading[SETTINGS_LOADING_MAX_LEN + 1];
extern char g_eyesStyle[SETTINGS_EYES_STYLE_MAX_LEN + 1];
extern char g_accessToken[SETTINGS_ACCESS_TOKEN_MAX_LEN + 1];
extern WifiNetworkList g_wifiNetworks;
extern uint8_t g_servoMin[SETTINGS_SERVO_COUNT];
extern uint8_t g_servoMax[SETTINGS_SERVO_COUNT];
extern char g_rgbOrder[SETTINGS_RGB_ORDER_MAX_LEN + 1];
extern bool g_oledRotate180;

void setHostnameCache(char* dest, const char* src);
void setLoadingCache(char* dest, const char* src);
void setEyesStyleCache(char* dest, const char* src);
void setAccessTokenCache(char* dest, const char* src);
void setRgbOrderCache(char* dest, const char* src);
void logAccessTokenState();
void logSettingsSnapshot(const char* prefix);
