# Microcontrollers and ESP32

Your laptop runs Linux, Chrome, and seventeen Slack notifications. A microcontroller runs *one program*, forever, with no OS you recognize. That gap — between "real computer" and "chip that drives GPIO" — is where most software engineers stall. This chapter closes it. Tiny Engineer uses an **ESP32-C3** as its instance; the ideas apply to Arduino, STM32, and most hobby boards.

---

## MCU vs SBC vs your laptop

| | Microcontroller (MCU) | Single-board computer (SBC) | Laptop |
| --- | --- | --- | --- |
| Examples | ESP32, Arduino, STM32 | Raspberry Pi, BeagleBone | What you're reading this on |
| OS | Bare-metal or RTOS; often "Arduino" | Full Linux | Full OS |
| Boot time | Milliseconds | Seconds | Minutes if updates pending |
| GPIO pins | Yes, direct | Yes, but often through OS layers | Basically no |
| Real-time I/O | Designed for it | Possible, awkward | Not the point |
| Power | Milliwatts to low watts | Watts | Tens of watts |

Tiny Engineer needs Wi-Fi, five servos, a display, and audio on a desk robot budget. An MCU with built-in radio fits. A Pi would work but adds OS overhead, boot time, and power for no gain here.

---

## What firmware is

**Firmware** is the program stored in flash on the chip. It runs on boot. There's no `git pull` on the device — you **build** on your computer and **upload** (flash) the binary over USB.

> **If you've written backend code…** Flashing firmware is deploying a single static binary to a device with no package manager. Serial monitor is `stdout`. There's no SSH — you re-flash or use HTTP once Wi-Fi works.

**Flash** = non-volatile storage (survives power-off). Holds your program and, on this project, LittleFS files (WAV sounds).

**RAM** = working memory. Much smaller than flash. Gone on reboot.

---

## GPIO — the hardware API

**GPIO** (General Purpose Input/Output) pins are wires the firmware controls or reads. On the wire that is literally **zeros and ones** as voltages (see [Ch. 01](01-electricity-and-units.md)):

- **Output HIGH / 1** ≈ 3.3 V on that pin (on ESP32)
- **Output LOW / 0** ≈ 0 V (GND)
- **Input** — read whether the other side is holding the line high or low

Blink an LED? Firmware toggles one pin between 0 and 1. Same idea scales up: **communication protocols are patterns of zeros and ones on one or more pins over time** — serial (UART), I2C, SPI, I2S, even USB on this chip. Libraries hide the bit timing; underneath it's still GPIO-level signaling.

You don't get unlimited pins. Each pin might be reserved for flash, USB, or boot strapping. Scarcity is real — see [pinout.md](../hardware/pinout.md) for the full map.

---

## Boot, reset, and upload

**RESET** — reboots the chip (like killing and restarting a process).

**BOOT** — strapping pin; hold during reset to enter serial upload mode if auto-reset fails.

**Upload** — PlatformIO sends firmware over USB. On ESP32-C3, that's **native USB** on specific GPIO pins (not a separate USB-UART chip).

If upload fails: try another cable (data, not charge-only), another port, hold BOOT, check drivers. Details in [getting-started.md § Flash](../getting-started.md#4-flash).

---

## What "Arduino on ESP32" means

This project uses **PlatformIO** with the **Arduino framework** for ESP32. You get `setup()` / `loop()`, familiar libraries (`Wire` for I2C, etc.), and Espressif's Wi-Fi stack underneath.

You're not writing bare-metal RISC-V assembly. You're writing C++ that compiles to a binary the chip runs directly. `platformio.ini` picks the board target and libraries.

---

## ESP32 family (just enough context)

Espressif makes many ESP32 variants. Tiny Engineer uses **ESP32-C3**:

- **RISC-V** single-core (not the older dual-core Xtensa ESP32)
- **Wi-Fi 2.4 GHz only** — no 5 GHz; your router must expose 2.4 GHz for setup
- **BLE** exists on the silicon; this project's firmware doesn't use it
- Native **USB** — no external CP2102 required when wired correctly

---

## In Tiny Engineer

### The board: Waveshare ESP32-C3-Zero

Not a generic breadboard devkit. It's a compact module with castellated edges, a ceramic antenna, and an onboard RGB LED.

**PlatformIO naming trap:** `board = esp32-c3-devkitm-1` in `platformio.ini` is a **build target name**, not the physical board in your hand. The physical board is the C3-Zero. Same chip, different pin breakout — the project's [pinout.md](../hardware/pinout.md) and [components.md](../hardware/components.md) describe *this* board.

### What the ESP32 does here

| Job | How |
| --- | --- |
| Wi-Fi + HTTP API | Agent integration, web UI, `/anim`, `/test/*` |
| I2C master | Talks to PCA9685 (servos) and OLED (display) |
| I2S master | Streams audio samples to MAX98357A |
| Status RGB | Onboard WS2812 on GPIO10 — boot green, anim white, error red |
| "Sensor" | On-die CPU temperature in `/health` — that's it, no IMU |

### Pins you'll hear about (conceptual)

The firmware dedicates most exposed GPIOs. Rough mental map:

- **GP0 / GP1** — I2C (SDA / SCL) to PCA9685 and OLED
- **GP2–GP4** — I2S (BCLK, LRC, DIN) to audio amp
- **GP9** — BOOT button
- **GP10** — onboard RGB LED
- **GP18 / GP19** — native USB D− / D+ (wired to Adafruit 5993 breakout when assembled)

Many other GPIOs are used internally for flash and aren't available. Don't reassign USB pins if you flash over that cable.

Full allocation: [pinout.md](../hardware/pinout.md).

### USB: onboard vs breakout

The C3-Zero has a USB-C port on the board, but **when the robot is assembled**, flash and serial go through the **Adafruit 5993 USB-C breakout** wired to GPIO18/19. One cable on the robot's exterior for power and programming.

### The RGB LED is your boot log

Before Wi-Fi works, the LED tells you state: dim green while booting, solid red if init failed (e.g. PCA9685 missing). It's the hardware equivalent of a health-check endpoint you can see without serial.

### Ceramic antenna — don't bury it

Wi-Fi is how coding agents talk to the robot. The C3-Zero uses a **ceramic antenna** on the board. Metal or thick plastic pressed against it kills range. When you design or assemble the shell, leave a keep-out zone. More in [Ch. 06](06-3d-printing-and-mechanical-build.md).

### Flash workflow

Build, upload, monitor — copy-paste lives in [getting-started.md § Flash](../getting-started.md#4-flash). This chapter stops at *what* the chip is; that page is *how* to deploy.

---

**Next:** [Wiring craft and schematics](03-wiring-craft-and-schematics.md)

**Reference:** [hardware/components.md](../hardware/components.md) · [hardware/pinout.md](../hardware/pinout.md) · [getting-started.md § Flash](../getting-started.md#4-flash)
