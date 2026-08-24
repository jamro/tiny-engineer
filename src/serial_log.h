#pragma once

#include <Arduino.h>

#include "settings.h"

inline void serialLogPrintln() {
  if (settingsSerialLogEnabled()) {
    Serial.println();
  }
}

template <typename T>
inline void serialLogPrint(const T& value) {
  if (settingsSerialLogEnabled()) {
    Serial.print(value);
  }
}

template <typename T>
inline void serialLogPrint(const T& value, int baseOrDecimals) {
  if (settingsSerialLogEnabled()) {
    Serial.print(value, baseOrDecimals);
  }
}

template <typename T>
inline void serialLogPrintln(const T& value) {
  if (settingsSerialLogEnabled()) {
    Serial.println(value);
  }
}

template <typename T>
inline void serialLogPrintln(const T& value, int baseOrDecimals) {
  if (settingsSerialLogEnabled()) {
    Serial.println(value, baseOrDecimals);
  }
}
