#pragma once

#include <cstddef>

// USB CDC framed control plane. Call serialLinkWatchUsb() right after
// Serial.begin(); serialLinkBegin() after settings init; poll from loop().

// Rearm USB Serial/JTAG RX. The OUT interrupt fires only when the FIFO goes
// from empty to not-empty, so bytes left by boot or a host bus reset (Mac
// restart, replug) silence later sync frames until that FIFO is drained.
void serialLinkWatchUsb();
void serialLinkBegin();
void serialLinkPoll();
// True once the link may emit LOG frames (settings loaded, framer ready).
bool serialLinkReady();
void serialLinkEmitLog(const char* text);

// Best-effort device-initiated event. Drops the frame when CDC TX cannot take
// the packed COBS frame; never blocks.
void serialLinkEmitEvent(const char* json, size_t jsonLen);
