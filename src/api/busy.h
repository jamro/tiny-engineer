#pragma once

// Exclusive lock for long-running ops (WAV play). Short ops do not take it.

bool apiBusyTryAcquire();
void apiBusyRelease();
bool apiBusyHeld();
