#pragma once

#include <cstdint>

// Watches the I2C bus the PCA9685 and the OLED share. The SSD1306 driver
// ignores failed writes and the firmware used to ignore the PCA9685 driver's
// write status, so a dead bus left the robot frozen while every write waited
// out its timeout and starved audio and HTTP.

// Frees a bus a device still holds from before a reset, then starts Wire.
void initI2cBus();

// Starts the checks. Call once the PCA9685 and the OLED are set up.
void beginI2cHealth();

// Checks each chip while it is up and sets it up again while it is down.
// Cheap enough for every tick.
void updateI2cHealth();

// False while the PCA9685 cannot be used: skip servo writes.
bool i2cServoDriverUp();

// A servo write failed. Servo writes stop until the chip has been set up again.
void i2cNoteServoWriteFailed();

struct I2cDeviceHealth {
  bool ok;
  uint32_t faults;
  uint32_t recoveries;
  uint32_t lastFaultMs;
};

struct I2cHealth {
  bool ok;
  I2cDeviceHealth servoDriver;
  I2cDeviceHealth oled;
};

I2cHealth i2cHealth();
