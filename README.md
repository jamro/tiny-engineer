# Tiny Engineer

Tiny Engineer is a small Wi-Fi desk robot that acts out your AI coding assistant while it works.

When you ask Cursor, Claude Code, or another agent to do a task, the model is busy somewhere in the cloud. On your desk, Tiny Engineer pretends it is the one doing the job: typing on a keyboard, leaning in to read, pausing to think, then getting back to work.

![Tiny Engineer at its desk](docs/tiny-engineer-preview.png)

## The idea

AI tools already feel a bit like a coworker in another room. Tiny Engineer puts a tiny coworker on the desk.

You keep using the tools you already use. The robot does not write the code. It performs the *waiting*: the stretch of time between “please fix this” and “here is the patch.” Head turns toward a miniature laptop. Arms tap the keys. The pose shifts when the model is “thinking.” It is theatre, on purpose.

The point is not to fake intelligence. The point is to make the invisible stretch of inference visible, and a little bit funny.

Later firmware will take live status over Wi-Fi (agent running, waiting, done) and map it to those gestures. Right now the repo is the hardware and a bring-up test: ESP32-C3, five servos, OLED, speaker, so the body can move, show a line of text, and make a sound.

## Hardware

Controller is a **Waveshare ESP32-C3-Zero**. Servos, display, and audio wiring live in [`docs/hardware/`](docs/hardware/README.md).

## Build and flash

Arduino firmware, built with [PlatformIO](https://platformio.org/). `src/main.cpp` is the hardware bring-up test: onboard RGB, SSD1306 OLED, PCA9685 (5 servos), MAX98357A (I2S tone).

Prerequisites:

- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/installation.html), or the PlatformIO extension in VS Code / Cursor
- ESP32-C3-Zero connected over USB

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

## License

MIT. See [LICENSE](LICENSE).
