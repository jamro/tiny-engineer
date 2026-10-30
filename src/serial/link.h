#pragma once

#include <cstddef>

// USB CDC framed control plane. Call after settings init; poll from loop().

void serialLinkBegin();
void serialLinkPoll();
// True once the link may emit LOG frames (settings loaded, framer ready).
bool serialLinkReady();
void serialLinkEmitLog(const char* text);

// Best-effort device-initiated event. Drops the frame when CDC TX cannot take
// the packed COBS frame; never blocks.
void serialLinkEmitEvent(const char* json, size_t jsonLen);
