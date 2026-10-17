#pragma once

// USB CDC framed control plane. Call after settings init; poll from loop().

void serialLinkBegin();
void serialLinkPoll();
// True once the link may emit LOG frames (settings loaded, framer ready).
bool serialLinkReady();
void serialLinkEmitLog(const char* text);
