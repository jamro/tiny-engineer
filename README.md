# tiny-engineer

Arduino firmware for **ESP32-C3 DevKitM-1**, built with [PlatformIO](https://platformio.org/).

`src/main.cpp` runs a hardware bring-up test: onboard RGB, SSD1306 OLED, PCA9685 (5 servos), and MAX98357A (I2S tone).

## Prerequisites

- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/installation.html), or the PlatformIO extension in VS Code / Cursor
- ESP32-C3 DevKitM-1 connected over USB

## Usage

From the project root:

```bash
pio run                 # build
pio run -t upload       # flash the board
pio device monitor      # serial monitor (115200 baud)
```

Build and flash in one step:

```bash
pio run -t upload && pio device monitor
```

If several serial ports are present, pick one:

```bash
pio device list
pio run -t upload --upload-port /dev/cu.usbserial-XXXX
pio device monitor --port /dev/cu.usbserial-XXXX
```

Board and baud rate live in `platformio.ini`. The platform package is [pioarduino](https://github.com/pioarduino/platform-espressif32) (Arduino-ESP32 3.x) so `ESP_I2S` and `rgbLedWrite` are available.
