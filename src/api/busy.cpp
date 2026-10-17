#include "api/busy.h"

namespace {

bool g_busy = false;

}  // namespace

bool apiBusyTryAcquire() {
  if (g_busy) {
    return false;
  }
  g_busy = true;
  return true;
}

void apiBusyRelease() {
  g_busy = false;
}

bool apiBusyHeld() {
  return g_busy;
}
