#pragma once

#include "api/request.h"

void apiOpAuthGet(const ApiRequest& req, ApiResponse& resp);
void apiOpHealthGet(const ApiRequest& req, ApiResponse& resp);

void apiOpAnimGet(const ApiRequest& req, ApiResponse& resp);
void apiOpAnimPost(const ApiRequest& req, ApiResponse& resp);

void apiOpSettingsGet(const ApiRequest& req, ApiResponse& resp);
void apiOpSettingsPost(const ApiRequest& req, ApiResponse& resp);
void apiOpSettingsReset(const ApiRequest& req, ApiResponse& resp);

// Shared settings JSON body (used by HTTP PUT /settings/wifi).
void apiFillSettingsResponse(
  ApiResponse& resp,
  bool rebootRequired,
  bool wifiConnectSuccess
);

void apiOpTestAudio(const ApiRequest& req, ApiResponse& resp);
void apiOpTestBell(const ApiRequest& req, ApiResponse& resp);
void apiOpTestScreen(const ApiRequest& req, ApiResponse& resp);
void apiOpTestMovement(const ApiRequest& req, ApiResponse& resp);
void apiOpTestLed(const ApiRequest& req, ApiResponse& resp);
void apiOpTestServo(const ApiRequest& req, ApiResponse& resp);

void apiOpSetupServo(const ApiRequest& req, ApiResponse& resp);
void apiOpSetupLed(const ApiRequest& req, ApiResponse& resp);
void apiOpSetupAudio(const ApiRequest& req, ApiResponse& resp);
void apiOpSetupOled(const ApiRequest& req, ApiResponse& resp);
