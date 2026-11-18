#pragma once

#include <ArduinoJson.h>
#include <cstddef>

// Shared LCD extension JSON handlers (HTTP + serial). Returns HTTP-like status.
// On failure fills err (NUL-terminated); on success fills out.

int lcd1602ApiTextGet(JsonDocument& out);
int lcd1602ApiTextPost(JsonObjectConst body, JsonDocument& out, char* err, size_t errLen);
int lcd1602ApiTextDelete(JsonDocument& out);
int lcd1602ApiBrightnessGet(JsonDocument& out);
int lcd1602ApiBrightnessPost(JsonObjectConst body, JsonDocument& out, char* err, size_t errLen);
int lcd1602ApiColorGet(JsonDocument& out, char* err, size_t errLen);
int lcd1602ApiColorPost(JsonObjectConst body, JsonDocument& out, char* err, size_t errLen);
