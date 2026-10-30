# Design

## Context

See proposal.md for why a PCBA test session exists. The product image in `[env:esp32-c3-devkitm-1]` boots Wi-Fi, HTTP, LittleFS, and the COBS serial stack from `src/main.cpp`. `[env:expression-demo]` already shows the opt-in pattern: its own `main`, a `build_src_filter` that drops `src/`, and none of the product `extra_scripts`. Pin numbers live in `include/pins.h`. Servo columns on U2 are head=0, neck=1, arm-l=2, arm-r=3, body=4, in that physical order from the HEAD silkscreen toward BODY.

Whoever runs the test uses one golden ESP32-C3-Zero, one 3-pin LED plug (red on the column 5 V pin, blue on PWM), one speaker on J1, and a PC whose OS is not known ahead of time. The script and the page say the test is compatible with main control board rev 3.

## Goals / Non-Goals

**Goals:**

- One opt-in env whose image cannot be produced by the default `pio run`.
- A line protocol the host can follow without parsing COBS.
- Operator keys that sit on the same physical keys on a US and a Polish keyboard.
- Both hosts stay up across USB unplug when the cable moves to the next board.

**Non-Goals:**

- Judging the plug LEDs or the speaker from firmware. The operator is the sensor. The onboard LED is only an alive lamp.
- Measuring pulse width, eFuse trip current, or J2/J3/J4.
- A runtime switch inside the product image.
- Serving the tester from the robot `ui/` or from the test image.

## Decisions

### Separate env, own main

`[env:pcba-test]` copies the `expression-demo` shape: same platform, board, and USB CDC flags, `build_src_filter` pointing at `examples/pcba-test/main.cpp`, no `embed_ui`, `copy_assets`, `upload_fs_after_upload`, or `park_servos_before_upload`. It links Adafruit's PWM library only. `include/pins.h` is on the include path for GPIO numbers and `PCA9685_ADDRESS`.

Alternative: `-D PCBA_TEST` inside `src/main.cpp`. Rejected. The product boot would still be one edit away from running on a module that should boot the product, and the test image would still compile Wi-Fi and the UI scripts unless those scripts were also stripped.

Alternative: a second repository. Rejected. Pin and channel numbers would drift from `pins.h`.

### Plain lines, not the product serial protocol

The image prints one token line at a time and reads one operator key. Banner `PCBA-TEST v1`. Steps `STEP tone`, then `STEP servos`, then `STEP oe`. Failures `FAIL <step>`. Menus are the single letters below. Verdict `RESULT PASS` or `RESULT FAIL` plus the failed step names separated by spaces. Idle is `STOP` after `q`. A key that does not apply to the current state is ignored. Enter on the wire is a single line feed.

Keys: Enter accepts, `n` rejects a step, `r` retries, `c` continues, `a` restarts the board, `q` stops after RESULT. After RESULT the next board is an unplug: USB power loss resets the module, and the host attaches when the port returns. `n` after RESULT is ignored. `y` is avoided because it moves on a Polish keyboard.

Alternative: COBS frames shared with `src/serial/`. Rejected. The test host would need the product codec, and the product spec forbids raw text on the robot image. This image is not that image.

### Step behavior

PCA probe uses `Wire` at `0x40` before any step. Failure prints `FAIL pca` and `RESULT FAIL pca`, with no `r`/`c`/`a` menu. The board ends the same way as any other verdict: unplug for the next one, `q` to stop.

Tone is a generated sine on the I2S pins (GP2, GP3, GP4), repeated until Enter or `n`. During that step `+` raises the level and `-` lowers it, across ten steps. The middle step is the bench default. Other steps ignore those keys. It does not call `playTone()` in `src/audio/audio.cpp`, which reads NVS volume.

The servos step sweeps PWM duty on channels 0 through 4 together so every blue LED can pulse, with OE enabled (GP5 low). The operator plugs the testing board into one column at a time during that one step. The last step drives OE high so the pins turn off, while PWM keeps pulsing on channels 0 through 4. The 5 V rail stays on, so only the blue LED goes out. Restarting the board enables OE again. Silk-side pin is blue (SIG), middle pin is red (5 V), square pad is GND. Each column expects red on and blue pulsing. `tone` is the ear on J1; lamps are ignored.

`r` repeats the current step and drops a fail recorded for that attempt. `c` appends the step name and advances. `a` clears the list and returns to the connected confirmation without a power cycle. Enter starts the tone again. On a step `FAIL` the host shows `r`, `c`, and `a` with the step name. `FAIL pca` does not.

The onboard WS2812 (`RGB_LED_PIN`, GPIO10) is turned steady white once the image is up and left there through steps, the failure menu, `FAIL pca`, `RESULT`, and `STOP`. The three channels are equal, so `rgb_order` is not read and NVS is not opened. Red and green are not used. The lamp does not blink and does not encode pass or fail. Removing USB power is what turns it off.

Alternative: blue, to dodge an R/G swap. Rejected. The product already allows `RBG`, `GBR`, `BRG`, and `BGR`, and those put blue on the red or green wire. White survives every order.

### On-screen coaching

The wire stays the token lines. Each host adds the same English sentences for the steps. A waiting step does not print the `STEP` token. It shows `Enter = good` and `n = bad` in the box footer, and its place in the three. The page shows the banner sentence. The script does not print it. The procedure card already says the lamp is not the verdict.

| Token | On screen |
| --- | --- |
| banner | Page only. The module LED stays steady white while the test image has power. That lamp is not the board verdict. |
| `1/3` `tone` | Speaker on the JST speaker port. Expect a repeating tone. `+` louder, `-` quieter. The card shows the level. |
| `2/3` `servos` | Signal is on all five servo outputs. Take the testing board. Plug it into one column at a time. Columns are HEAD, NECK, ARM L, ARM R, and BODY. Check the LEDs on every column. Red stays on and blue pulses. |
| `3/3` OE OUTPUT ENABLE | The red LED stays on. Only the blue LED goes out. No need to test each servo column. |

A step `FAIL` repeats that step's expected effect and shows `r = try this step again`, `c = keep the fail, go on`, and `a = start this board over`. `FAIL pca` shows `PCA9685 did not answer. This board fails before the speaker test.` and does not show `r`, `c`, or `a`.

### Two hosts, one line protocol

`scripts/pcba_test.py` uses Python 3 and `pyserial`. That install exists for Windows, macOS, and Linux. On start it prints `https://tinyengineer.dev/pcba_test` as the alternate tester. The script picks a single CDC port (or `--port`), prints the English coaching as the large line, shows `Enter = good` and `n = bad` in the box footer, shows `r`, `c`, and `a` when the line is a step `FAIL`, and writes keys to the port. On port loss it waits and opens the port that comes back.

The alternate host is `web/pcba_test/index.html`, published at `https://tinyengineer.dev/pcba_test` the same way `web/flash/` is `/flash/`. A request without the trailing slash redirects to the slash form. The page opens the golden module with Web Serial, clears DTR and RTS, writes `?`, and speaks the same lines and keys. It is not a route in the robot `ui/`. The test image does not start HTTP.

On load the page checks `isSecureContext` and `navigator.serial`, the same gate as `web/js/flash-status.js`. If either fails, the page says this browser cannot run the test, names `python3 scripts/pcba_test.py`, and shows no connect control. Chrome or Edge on the HTTPS site passes. The first open needs a click, because a serial grant comes only from a user gesture. The page then keeps that grant and reopens it when the cable moves, without a reload. After the port is open the page hides Connect and shows Disconnect. Disconnect closes the port and reloads the page. After the link drops, Connect comes back and the click asks for the port again. The page also names `python3 scripts/pcba_test.py` and links the procedure doc on GitHub.

No HTML page under `web/` other than this one links to `/pcba_test`. The procedure card `docs/hardware/pcba-test.md` states the bench, the plug facing, that the servos step pulses all five columns while the operator moves the plug with no key between columns, the red/blue look, the steady white module LED, J1, the keys, unplug for the next board, that the golden module stays with the tester, `python3 scripts/pcba_test.py`, and `https://tinyengineer.dev/pcba_test`.

Alternative: a `file://` HTML page. Rejected. Web Serial is blocked outside a secure context. Alternative: a route on the product panel or on the test image. Rejected. The test image does not start HTTP, and the product panel is the robot UI. Alternative: three native binaries. Deferred. The script covers every OS; the page covers Chromium on the site.

### Golden module stays with the tester

The test image is uploaded with `pio run -e pcba-test -t upload` onto the fixture module. That module stays with the person running the test. Both hosts say the ESP32 must already run the test firmware and point at the procedure section for how to get one. This change does not flash the product image onto a produced board.

## Risks / Trade-offs

- [Operator skips a column] → The servos step pulses all five outputs at once, and the card says to move the plug across every column before Enter. A missed column still logs as pass if the operator accepts.
- [Plug reversed 180°] → SIG and GND swap. Blue stays dark, so the step fails if the operator follows the card. The 1 kΩ on the red LED limits current into the PWM pin.
- [I2S init succeeds with a dead amplifier] → Pass/fail for `tone` is the operator's ear, not the init return code.
- [Windows COM number changes after replug] → The script treats disappearance as "wait" and attaches to the new CDC port instead of exiting. The page reopens the same granted device, because the golden module does not change.
- [Browser has no Web Serial] → The page names the Python command and does not offer a port connection.
- [Test image flashed onto a module that should run the product] → Default env is unchanged. The pcba-test upload command names `-e pcba-test`. The image banner is `PCBA-TEST`, not `TINY ENGINEER`.

## Migration Plan

No product NVS, HTTP, or serial change. Rollback is deleting the env and the example. A module that should run the product is reflashed with the product env before it leaves the test setup.

## Open Questions

None that change the session. The on-bench LED wiring drawing stays outside this change.
