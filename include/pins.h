#pragma once

#include <cstdint>

// =====================================================
// PINOUT
// =====================================================

// I2C -> PCA9685 + OLED
constexpr int I2C_SDA = 0;   // GP0
constexpr int I2C_SCL = 1;   // GP1

// I2S -> MAX98357A
constexpr int I2S_BCLK = 2;  // GP2
constexpr int I2S_LRC  = 3;  // GP3
constexpr int I2S_DIN  = 4;  // GP4

// Built-in WS2812 RGB
constexpr int RGB_LED_PIN = 10;

// =====================================================
// AUDIO
// =====================================================

constexpr int SAMPLE_RATE = 44100;

// =====================================================
// PCA9685 / SERVOS
// =====================================================

constexpr uint8_t PCA9685_ADDRESS = 0x40;

constexpr int SERVO_COUNT = 5;

constexpr int SERVO_CHANNELS[SERVO_COUNT] = {
  0, 1, 2, 3, 4
};

constexpr int SERVO_STEP_MS = 20;

// Smooth single-servo move rate for /test/servo
constexpr float SERVO_SPEED_DEG_S = 40.0f;

// Servo pulse range
constexpr int SERVO_MIN_US = 800;
constexpr int SERVO_MAX_US = 2200;

// Safe test range
constexpr float SERVO_LOW    = 75.0f;
constexpr float SERVO_CENTER = 90.0f;
constexpr float SERVO_HIGH   = 105.0f;

// =====================================================
// OLED
// =====================================================

constexpr int OLED_WIDTH  = 128;
constexpr int OLED_HEIGHT = 32;
constexpr uint8_t OLED_ADDRESS = 0x3C;
