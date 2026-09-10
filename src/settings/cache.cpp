#include "settings/internal.h"
#include "serial_log.h"
#include "settings/settings.h"

#include <cstring>

uint32_t g_sleepTimeoutMin = SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN;
char g_hostname[SETTINGS_HOSTNAME_MAX_LEN + 1] = {};
char g_bootHostname[SETTINGS_HOSTNAME_MAX_LEN + 1] = {};
uint8_t g_volume = SETTINGS_DEFAULT_VOLUME;
bool g_welcome = SETTINGS_DEFAULT_WELCOME;
bool g_serialLog = SETTINGS_DEFAULT_SERIAL_LOG;
uint32_t g_continuousTimeoutMin = SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN;
char g_loading[SETTINGS_LOADING_MAX_LEN + 1] = {};
char g_accessToken[SETTINGS_ACCESS_TOKEN_MAX_LEN + 1] = {};
char g_wifiSsid[SETTINGS_WIFI_SSID_MAX_LEN + 1] = {};
char g_wifiPassword[SETTINGS_WIFI_PASSWORD_MAX_LEN + 1] = {};
uint8_t g_servoMin[SETTINGS_SERVO_COUNT] = {};
uint8_t g_servoMax[SETTINGS_SERVO_COUNT] = {};
char g_rgbOrder[SETTINGS_RGB_ORDER_MAX_LEN + 1] = {};
bool g_oledRotate180 = SETTINGS_DEFAULT_OLED_ROTATE_180;

void setHostnameCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_HOSTNAME_MAX_LEN);
  dest[SETTINGS_HOSTNAME_MAX_LEN] = '\0';
}

void setLoadingCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_LOADING_MAX_LEN);
  dest[SETTINGS_LOADING_MAX_LEN] = '\0';
}

void setAccessTokenCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_ACCESS_TOKEN_MAX_LEN);
  dest[SETTINGS_ACCESS_TOKEN_MAX_LEN] = '\0';
}

void setWifiSsidCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_WIFI_SSID_MAX_LEN);
  dest[SETTINGS_WIFI_SSID_MAX_LEN] = '\0';
}

void setWifiPasswordCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_WIFI_PASSWORD_MAX_LEN);
  dest[SETTINGS_WIFI_PASSWORD_MAX_LEN] = '\0';
}

void setRgbOrderCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_RGB_ORDER_MAX_LEN);
  dest[SETTINGS_RGB_ORDER_MAX_LEN] = '\0';
}

void logAccessTokenState() {
  serialLogPrint(" access_token=");
  serialLogPrint(g_accessToken[0] != '\0' ? "set" : "unset");
}

void logSettingsSnapshot(const char* prefix) {
  serialLogPrint(prefix);
  serialLogPrint(" sleep_timeout=");
  serialLogPrint(g_sleepTimeoutMin);
  serialLogPrint("min hostname=");
  serialLogPrint(g_hostname);
  serialLogPrint(" volume=");
  serialLogPrint(g_volume);
  serialLogPrint(" welcome=");
  serialLogPrint(g_welcome ? "on" : "off");
  serialLogPrint(" serial_log=");
  serialLogPrint(g_serialLog ? "on" : "off");
  serialLogPrint(" continuous_timeout=");
  serialLogPrint(g_continuousTimeoutMin);
  serialLogPrint("min loading=");
  serialLogPrint(g_loading);
  logAccessTokenState();
  serialLogPrint(" wifi=");
  serialLogPrint(g_wifiSsid[0] != '\0' ? "configured" : "unset");
  serialLogPrint(" servos=");

  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    if (i > 0) {
      serialLogPrint(";");
    }

    serialLogPrint(g_servoMin[i]);
    serialLogPrint("-");
    serialLogPrint(g_servoMax[i]);
  }

  serialLogPrint(" rgb_order=");
  serialLogPrint(g_rgbOrder);
  serialLogPrint(" oled_rotate_180=");
  serialLogPrint(g_oledRotate180 ? "on" : "off");
  serialLogPrintln();
}
