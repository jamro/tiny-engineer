#pragma once

#include <cstdint>

#include "settings.h"

extern uint32_t g_sleepTimeoutMin;
extern char g_hostname[SETTINGS_HOSTNAME_MAX_LEN + 1];
extern char g_bootHostname[SETTINGS_HOSTNAME_MAX_LEN + 1];
extern uint8_t g_volume;
extern bool g_welcome;
extern uint32_t g_continuousTimeoutMin;
extern char g_loading[SETTINGS_LOADING_MAX_LEN + 1];
extern char g_accessToken[SETTINGS_ACCESS_TOKEN_MAX_LEN + 1];

void setHostnameCache(char* dest, const char* src);
void setLoadingCache(char* dest, const char* src);
void setAccessTokenCache(char* dest, const char* src);
void logAccessTokenState();
