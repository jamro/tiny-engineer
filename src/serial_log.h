#pragma once

#include <Arduino.h>

#include "serial/link.h"
#include "settings/settings.h"

namespace serial_log_detail {

inline String& line() {
  static String buffer;
  return buffer;
}

inline void flushLine() {
  if (!settingsSerialLogEnabled()) {
    line() = "";
    return;
  }
  if (serialLinkReady()) {
    serialLinkEmitLog(line().c_str());
  }
  line() = "";
}

}  // namespace serial_log_detail

inline void serialLogPrintln() {
  serial_log_detail::flushLine();
}

template <typename T>
inline void serialLogPrint(const T& value) {
  if (!settingsSerialLogEnabled()) {
    return;
  }
  serial_log_detail::line() += String(value);
}

template <typename T>
inline void serialLogPrint(const T& value, int baseOrDecimals) {
  if (!settingsSerialLogEnabled()) {
    return;
  }
  serial_log_detail::line() += String(value, baseOrDecimals);
}

template <typename T>
inline void serialLogPrintln(const T& value) {
  if (!settingsSerialLogEnabled()) {
    serial_log_detail::line() = "";
    return;
  }
  serial_log_detail::line() += String(value);
  serial_log_detail::flushLine();
}

template <typename T>
inline void serialLogPrintln(const T& value, int baseOrDecimals) {
  if (!settingsSerialLogEnabled()) {
    serial_log_detail::line() = "";
    return;
  }
  serial_log_detail::line() += String(value, baseOrDecimals);
  serial_log_detail::flushLine();
}
