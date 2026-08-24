#include "hardware/chip_temp.h"

#include <Arduino.h>
#include "driver/temperature_sensor.h"
#include "serial_log.h"

namespace {

temperature_sensor_handle_t g_tsens = nullptr;
bool g_ready = false;

}  // namespace

void initChipTemp() {
  temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);

  if (temperature_sensor_install(&cfg, &g_tsens) != ESP_OK) {
    serialLogPrintln("ERROR: chip temp sensor install failed");
    g_tsens = nullptr;
    g_ready = false;
    return;
  }

  if (temperature_sensor_enable(g_tsens) != ESP_OK) {
    serialLogPrintln("ERROR: chip temp sensor enable failed");
    g_tsens = nullptr;
    g_ready = false;
    return;
  }

  g_ready = true;
  serialLogPrintln("Chip temp sensor OK");
}

bool chipTempCelsius(float* out) {
  if (!g_ready || g_tsens == nullptr || out == nullptr) {
    return false;
  }

  return temperature_sensor_get_celsius(g_tsens, out) == ESP_OK;
}
