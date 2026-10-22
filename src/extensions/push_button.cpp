#include "extensions/push_button.h"

#include <Arduino.h>
#include <esp_timer.h>

#include "events/hub.h"
#include "extensions/push_button_logic.h"
#include "pins.h"

namespace {

portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
PushButtonSample g_queue[kPushButtonQueueDepth];
size_t g_head = 0;
size_t g_tail = 0;
size_t g_count = 0;
bool g_isrAttached = false;
PushButtonLogic g_logic;

void IRAM_ATTR onPinChange() {
  const PushButtonSample sample{
    digitalRead(PUSH_BUTTON_PIN) == HIGH,
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

}  // namespace

void pushButtonApply(bool enabled) {
  if (!enabled) {
    if (g_isrAttached) {
      detachInterrupt(digitalPinToInterrupt(PUSH_BUTTON_PIN));
      g_isrAttached = false;
    }
    pinMode(PUSH_BUTTON_PIN, INPUT);
    clearQueue();
    g_logic.apply(false, {});
    return;
  }

  pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
  const PushButtonSample sample{
    digitalRead(PUSH_BUTTON_PIN) == HIGH,
    static_cast<uint64_t>(esp_timer_get_time()),
  };
  clearQueue();
  g_logic.apply(true, sample);

  if (!g_isrAttached) {
    attachInterrupt(digitalPinToInterrupt(PUSH_BUTTON_PIN), onPinChange, CHANGE);
    g_isrAttached = true;
  }
}

void pushButtonPoll() {
  PushButtonSample sample;
  while (popSample(sample)) {
    const auto event = g_logic.accept(sample);
    if (!event) {
      continue;
    }

    if (*event == PushButtonEvent::Down) {
      publishEvent("button.down", "push-button");
    } else {
      publishEvent("button.up", "push-button");
    }
  }
}
