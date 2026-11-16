#pragma once

#include <cstdint>

// Watches the I2C bus the PCA9685 and the OLED share. The Adafruit drivers
// ignore failed writes, so without this a dead bus leaves the robot frozen
// while every write waits out its timeout and starves audio and HTTP.

// Frees a bus a device still holds from before a reset, then starts Wire.
void initI2cBus();

// Starts the checks. Call once the PCA9685 and the OLED are set up.
void beginI2cHealth();

// Probes the devices while the bus is up; restarts the bus and sets the
// devices up again while it is down. Cheap enough for every tick.
void updateI2cHealth(uint32_t now);

// False while the PCA9685 cannot be reached: skip servo writes.
bool i2cServoDriverUp();

struct I2cHealth {
  bool ok;
  bool servoDriver;
  uint32_t faults;
  uint32_t recoveries;
  uint32_t lastFaultMs;
};

I2cHealth i2cHealth();
