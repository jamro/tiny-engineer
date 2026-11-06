#include "extensions/push_button.h"

#include <Arduino.h>
#include <cstdio>
#include <esp_timer.h>

#include "audio/audio.h"
#include "events/hub.h"
#include "extensions/pin_claims_settings.h"
#include "extensions/push_button_logic.h"
#include "serial_log.h"
#include "settings/settings.h"
#include "settings/values.h"

namespace {

portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
PushButtonSample g_queue[kPushButtonQueueDepth];
size_t g_head = 0;
size_t g_tail = 0;
size_t g_count = 0;
bool g_isrAttached = false;
int g_activePin = -1;
PushButtonLogic g_logic;

void IRAM_ATTR onPinChange() {
  if (g_activePin < 0) {
    return;
  }

  const PushButtonSample sample{
    digitalRead(g_activePin) == HIGH,
    static_cast<uint64_t>(esp_timer_get_time()),
  };

  portENTER_CRITICAL_ISR(&g_mux);
  if (g_count == kPushButtonQueueDepth) {
    g_head = (g_head + 1) % kPushButtonQueueDepth;
    g_count--;
  }
  g_queue[g_tail] = sample;
  g_tail = (g_tail + 1) % kPushButtonQueueDepth;
  g_count++;
  portEXIT_CRITICAL_ISR(&g_mux);
}

bool popSample(PushButtonSample& out) {
  portENTER_CRITICAL(&g_mux);
  if (g_count == 0) {
    portEXIT_CRITICAL(&g_mux);
    return false;
  }
  out = g_queue[g_head];
  g_head = (g_head + 1) % kPushButtonQueueDepth;
  g_count--;
  portEXIT_CRITICAL(&g_mux);
  return true;
}

void clearQueue() {
  portENTER_CRITICAL(&g_mux);
  g_head = 0;
  g_tail = 0;
  g_count = 0;
  portEXIT_CRITICAL(&g_mux);
}

void detachActivePin() {
  if (g_isrAttached && g_activePin >= 0) {
    detachInterrupt(digitalPinToInterrupt(g_activePin));
    g_isrAttached = false;
  }
  if (g_activePin >= 0) {
    pinMode(g_activePin, INPUT);
  }
  g_activePin = -1;
}

const char* eventTypeName(PushButtonEvent event) {
  switch (event) {
    case PushButtonEvent::Down:
      return "button.down";
    case PushButtonEvent::Up:
      return "button.up";
    case PushButtonEvent::Click:
      return "button.click";
    case PushButtonEvent::DoubleClick:
      return "button.double_click";
    case PushButtonEvent::LongClick:
      return "button.long_click";
  }
  return "button.unknown";
}

const char* soundSettingFor(PushButtonEvent event) {
  switch (event) {
    case PushButtonEvent::Down:
      return settingsPushButtonSoundDown();
    case PushButtonEvent::Up:
      return settingsPushButtonSoundUp();
    case PushButtonEvent::Click:
      return settingsPushButtonSoundClick();
    case PushButtonEvent::DoubleClick:
      return settingsPushButtonSoundDoubleClick();
    case PushButtonEvent::LongClick:
      return settingsPushButtonSoundLongClick();
  }
  return SETTINGS_DEFAULT_PUSH_BUTTON_SOUND;
}

void publishButtonEvent(PushButtonEvent event) {
  publishEvent(eventTypeName(event), "push-button");
  tryStartButtonEventSound(soundSettingFor(event));
}

}  // namespace

void pushButtonApply(bool enabled, uint8_t pin) {
  const int nextPin = static_cast<int>(pin);

  if (!enabled) {
    detachActivePin();
    // Idle the configured pin even if ISR was never attached on it.
    pinMode(nextPin, INPUT);
    clearQueue();
    g_logic.apply(false, {});
    return;
  }

  if (g_isrAttached && g_activePin >= 0 && g_activePin != nextPin) {
    detachActivePin();
  }

  g_activePin = nextPin;
  pinMode(g_activePin, INPUT_PULLUP);
  const PushButtonSample sample{
    digitalRead(g_activePin) == HIGH,
    static_cast<uint64_t>(esp_timer_get_time()),
  };
  clearQueue();
  g_logic.apply(true, sample);

  if (!g_isrAttached) {
    attachInterrupt(digitalPinToInterrupt(g_activePin), onPinChange, CHANGE);
    g_isrAttached = true;
  }
}

void pushButtonApplyFromSettings() {
  ExtPinClaim claims[kExtPinClaimMaxProviders];
  const size_t n = pinClaimsCollect(g_settings, claims, kExtPinClaimMaxProviders);
  const uint8_t pin = settingsPushButtonPin();
  const bool enabled = settingsPushButton();

  if (enabled && pinClaimsPinIsContested(claims, n, pin)) {
    char conflict[96];
    pinClaimsConflict(claims, n, conflict, sizeof(conflict));
    char line[128];
    snprintf(line, sizeof(line), "Extensions: %s — leaving pin idle", conflict);
    serialLogPrintln(line);
    pushButtonApply(false, pin);
    return;
  }

  pushButtonApply(enabled, pin);
}

void pushButtonPoll() {
  PushButtonSample sample;
  while (popSample(sample)) {
    const auto edge = g_logic.accept(sample);
    if (const auto immediate = g_logic.takeImmediateGesture()) {
      publishButtonEvent(*immediate);
    }
    if (edge) {
      publishButtonEvent(*edge);
    }
  }

  const uint64_t nowUs = static_cast<uint64_t>(esp_timer_get_time());
  while (true) {
    const auto gesture = g_logic.tick(nowUs);
    if (!gesture) {
      break;
    }
    publishButtonEvent(*gesture);
  }
}
