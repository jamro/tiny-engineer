#pragma once

#include <Arduino.h>

#include "settings/settings.h"

namespace serial_log_detail {

inline bool& atLineStart() {
  static bool flag = true;
  return flag;
}

inline void ensurePrefix() {
  if (atLineStart()) {
    Serial.print("L ");
    atLineStart() = false;
  }
}

}  // namespace serial_log_detail

inline void serialLogPrintln() {
  if (settingsSerialLogEnabled()) {
    serial_log_detail::ensurePrefix();
    Serial.println();
    serial_log_detail::atLineStart() = true;
  }
}

template <typename T>
inline void serialLogPrint(const T& value) {
  if (settingsSerialLogEnabled()) {
    serial_log_detail::ensurePrefix();
    Serial.print(value);
  }
}

template <typename T>
inline void serialLogPrint(const T& value, int baseOrDecimals) {
  if (settingsSerialLogEnabled()) {
    serial_log_detail::ensurePrefix();
    Serial.print(value, baseOrDecimals);
  }
}

template <typename T>
inline void serialLogPrintln(const T& value) {
  if (settingsSerialLogEnabled()) {
    serial_log_detail::ensurePrefix();
    Serial.println(value);
    serial_log_detail::atLineStart() = true;
  }
}

template <typename T>
inline void serialLogPrintln(const T& value, int baseOrDecimals) {
  if (settingsSerialLogEnabled()) {
    serial_log_detail::ensurePrefix();
    Serial.println(value, baseOrDecimals);
    serial_log_detail::atLineStart() = true;
  }
}
