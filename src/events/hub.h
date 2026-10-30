#pragma once

#include <cstddef>
#include <cstdint>

// Outbound event hub: WebSocket (port 81 /events) + serial Evt frames.
// Producers call publishEvent from loop context only (never from an ISR).

void eventsHubBegin();
void eventsHubEnd();
void eventsHubPoll();
// Drop every connected /events client (e.g. after access_token rotation).
void eventsHubDisconnectAll();

// Builds {v:1,type,source,ms} and sends to every connected WebSocket client
// and as one serial Evt frame (best-effort: drop serial copy if TX is full).
void publishEvent(const char* type, const char* source);

// Reserved for later audio streaming; not called in this change.
void publishBinary(const uint8_t* data, size_t length);
