#pragma once

#include <Arduino.h>

#include "settings/settings.h"

namespace serial_log_detail {

inline bool& atLineStart() {
  static bool flag = true;
  return flag;
}

// True while serial API drains a binary /play body — skip L lines so TX
// cannot interleave with the host's framing expectations mid-transfer.
inline bool& binaryBusy() {
  static bool busy = false;
  return busy;
}

inline void ensurePrefix() {
  if (atLineStart()) {
    Serial.print("L ");
    atLineStart() = false;
  }
}

inline bool loggingActive() {
  return settingsSerialLogEnabled() && !binaryBusy();
}

}  // namespace serial_log_detail

inline void setSerialBinaryBusy(bool busy) {
  serial_log_detail::binaryBusy() = busy;
}

inline void serialLogPrintln() {
  if (serial_log_detail::loggingActive()) {
    serial_log_detail::ensurePrefix();
    Serial.println();
    serial_log_detail::atLineStart() = true;
  }
}

template <typename T>
inline void serialLogPrint(const T& value) {
  if (serial_log_detail::loggingActive()) {
    serial_log_detail::ensurePrefix();
    Serial.print(value);
  }
}

template <typename T>
inline void serialLogPrint(const T& value, int baseOrDecimals) {
  if (serial_log_detail::loggingActive()) {
    serial_log_detail::ensurePrefix();
    Serial.print(value, baseOrDecimals);
  }
}

template <typename T>
inline void serialLogPrintln(const T& value) {
  if (serial_log_detail::loggingActive()) {
    serial_log_detail::ensurePrefix();
    Serial.println(value);
    serial_log_detail::atLineStart() = true;
  }
}

template <typename T>
inline void serialLogPrintln(const T& value, int baseOrDecimals) {
  if (serial_log_detail::loggingActive()) {
    serial_log_detail::ensurePrefix();
    Serial.println(value, baseOrDecimals);
    serial_log_detail::atLineStart() = true;
  }
}
