#pragma once

#include <WebServer.h>

void handleIndex(WebServer& server);

void handleAuth(WebServer& server);
void handleHealth(WebServer& server);

void handleAnimGet(WebServer& server);
void handleAnimPost(WebServer& server);

// /play streams its body: handlePlayBody runs for each slice, handlePlayDone once it is consumed.
void handlePlayBody(WebServer& server);
void handlePlayDone(WebServer& server);

void handleSettingsGet(WebServer& server);
void handleSettingsPost(WebServer& server);
void handleSettingsReset(WebServer& server);
void handleSettingsWifiPut(WebServer& server);

void handleActionsGet(WebServer& server);
void handleActionsPut(WebServer& server);
void handleActionsTest(WebServer& server);

void handleAudioTest(WebServer& server);
void handleBellTest(WebServer& server);
void handleScreenTest(WebServer& server);
void handleMovementTest(WebServer& server);
void handleLedTest(WebServer& server);
void handleServoTest(WebServer& server);

void handleSetupServo(WebServer& server);
void handleSetupLed(WebServer& server);
void handleSetupAudio(WebServer& server);
void handleSetupOled(WebServer& server);

void handleServosPark(WebServer& server);
