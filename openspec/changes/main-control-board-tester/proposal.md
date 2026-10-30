# Proposal

## Why

Anyone who wants to check a produced main control board needs a repeatable pass/fail. The product firmware boots Wi-Fi, centers servos, and treats I2S init as success, so it cannot run this check. A PCBA test image and a small host program should walk one person through the ESP32 socket, the speaker, and each servo column.

## What Changes

- Add an opt-in PlatformIO env `pcba-test` with its own `main`. The default robot env stays unchanged. The test image is flashed once onto a golden ESP32-C3-Zero that stays with the person running the test.
- The image talks plain USB CDC lines at 115200. It does not speak the product COBS serial protocol, start Wi-Fi, or mount LittleFS. While it has USB power the onboard LED stays steady white, including after `q`. That lamp only means the test image is running.
- One operator, one keyboard. Silk-side pin is blue, middle pin is red, square pad is GND. The tone step is the speaker. The servos step puts signal on all five servo outputs at once. The operator plugs the testing board into HEAD, NECK, ARM L, ARM R, and BODY, one column at a time. Enter accepts a step. `n` rejects it.
- On every rejected step the image asks: retry the step (`r`), record the failure and continue (`c`), or restart the whole board (`a`). The tone repeats until Enter or `n`.
- After `RESULT PASS` or `RESULT FAIL`, the host stays open. Unplugging USB starts the next board. `q` ends the session. `n` after the verdict does nothing.
- Add a Python 3 host (`pyserial`) that finds the CDC port, shows the current step or the verdict, forwards those keys, and reconnects when the USB cable moves to the next board. On start it prints `https://tinyengineer.dev/pcba_test` as the alternate tester.
- Add that web host at `https://tinyengineer.dev/pcba_test` as an alternate path to the script. It uses the same CDC lines. No other site page links to it. If the browser has no Web Serial, or the page is not a secure context, the page says so and points at the Python script.
- Both hosts, in English, say what the current step tests, what to do, and the expected effect, and still show the wire token. Steps are numbered `1/3` through `3/3`. `FAIL pca` does not show `r`, `c`, or `a`.
- Out of scope: J2, J3, J4, OLED, Wi-Fi, a 2 A load, using the test image as the robot's everyday firmware, and serving this page from the robot UI or from the test image.

No **BREAKING** product HTTP, serial, or settings changes. The product binary is a different env.

## Capabilities

### New Capabilities

- `pcba-test`: PCBA test session on the golden module and the host: boot checks, tone, all servo columns at once, failure menu, board verdict, next board or stop.

### Modified Capabilities

- (none — the product `serial-protocol` stays on the robot image; the PCBA test image uses a separate line protocol)

## Impact

- **Firmware**: new env in `platformio.ini` (same pattern as `expression-demo`); new `examples/pcba-test/main.cpp`. Pin numbers come from `include/pins.h`. Product `src/main.cpp` is not linked and not edited for a runtime test mode.
- **Host**: one Python script under `scripts/`, dependency `pyserial`. Launch is `python3` plus that script on Windows, macOS, or Linux. The script prints `https://tinyengineer.dev/pcba_test` as the alternate. That page is `web/pcba_test/` on tinyengineer.dev (same shape as `/flash/`). Not a route in the robot `ui/`. Other pages under `web/` do not link to it. Both hosts print the same English step coaching next to the wire token.
- **Test hardware** (not built in this change): 3-pin female plug with a red LED on 5 V and a blue LED on PWM, plus one test speaker on J1.
- **Docs**: `docs/hardware/pcba-test.md`, the procedure card for the person running the test. It names the bench, the plug facing, the one servos step that pulses every column while the operator moves the plug with no key between columns, the lamp pattern, the steady white module LED, J1, the keys, unplug for the next board, the golden module staying with the tester, the Python command, and `https://tinyengineer.dev/pcba_test`. Product API docs stay as they are.
