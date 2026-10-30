# PCBA test

Check a produced main control board rev 3 with a golden ESP32-C3-Zero and one operator. The test is compatible with main control board rev 3. The product firmware cannot do this check. Power is the USB cable through the module. No servo supply.

The golden module stays with the tester. The ESP32 must already run the test firmware. How to get one is below. Do not ship that module on a board.

## Get an ESP32 with the test firmware

The ESP32 must already run the test firmware. The product firmware cannot run this test. The script and the page do not flash the module.

Seat a Waveshare ESP32-C3-Zero on a main control board. Match the USB mark. Plug a USB-C data cable into the board socket, not the ESP32 socket. From the repo root:

```bash
pio run -e pcba-test -t upload
```

When the test firmware is running, the onboard LED stays steady white. That lamp is not a board pass or fail. Before a module leaves the bench, flash the product image again.

The module LED stays steady white while the test image has power, including after a failed PCA probe, a verdict, and `q`. Steady white means the test image is running. It is not the board verdict. Judge the board by the ear on J1 and the plug lamps. Unplugging USB is what turns the module LED off.

## Run the script

Python 3 and pyserial. From the repo root:

```bash
python3 -m pip install pyserial
python3 scripts/pcba_test.py
```

The script prints a header, the web tester address, and a prepare card. It opens the board USB port. If that port is busy, the script says so and waits. Close the other program using the board. Keys match the page: Enter accepts a step, `n` rejects it. On SPEAKER, `+` is louder and `-` is quieter. After `RESULT`, unplug USB for the next board, or `q` to stop.

The other path is the web tester: [https://tinyengineer.dev/pcba_test](https://tinyengineer.dev/pcba_test). Chrome or Edge only. If the browser cannot open a serial port, the page says so and names `python3 scripts/pcba_test.py`.

USB-C data cable. Speaker on J1. Cards stay on screen. Act on the latest one.

## Testing board

The testing board seats on one column at a time. The silk-side pin is blue (SIG). The middle pin is red (5 V). Treat the square pad as GND.

During `servos`, signal is on all five servo outputs. Take the testing board and plug it into one column at a time: HEAD, NECK, ARM L, ARM R, and BODY. On every column, red stays on and blue pulses.

## What you see

The first card is prepare. Do that, then connect the board. The next card waits until the board is on USB, then waits for Enter before the first test step.

1. Take the board.
2. Plug in the ESP32. Match the USB mark on the board.
3. Plug the speaker into the JST speaker port.
4. Connect USB to the board. Use the board socket, not the ESP32.

Power-on is a short beep-beep, then silence. The next card is:

```
CONNECTED

The board is connected.

Enter = continue
```

Enter starts step `1/3`. `n` rejects a step. On a test step that is `Enter = good` and `n = bad`. On SPEAKER, `+` is louder and `-` is quieter. The card shows `+ = louder` and `- = quieter`, plus the level, from `Volume 1/10` through `Volume 10/10`. The tone keeps playing.

| Card | What you do | Look |
| --- | --- | --- |
| `1/3` `tone` | Speaker on the JST speaker port. `+` louder, `-` quieter. | A repeating tone until Enter or `n`. |
| `2/3` `servos` | Signal is on all five servo outputs. Plug the testing board into one column at a time: HEAD, NECK, ARM L, ARM R, and BODY. Check the LEDs on every column. | Red stays on and blue pulses. |
| `3/3` OE OUTPUT ENABLE | Look at the testing board on one column. No need to test each servo column. | The red LED stays on. Only the blue LED goes out. |

A column whose blue LED does not pulse is a failure. Judge every column before Enter.

## Keys

`n` shows `FAIL` and the menu `r`, `c`, `a`.

- `r` retries the step and drops that failure. On screen: `r = try this step again`.
- `c` keeps the failure and goes on. On screen: `c = keep the fail, go on`.
- `a` clears the board and returns to CONNECTED without unplugging. Enter starts the test again. On screen: `a = start this board over`.

After `RESULT PASS` or `RESULT FAIL`, the card says to unplug USB for the next board, or `q` to stop. The host stays open and attaches when the port returns. `q` prints `STOP`. `n` after the verdict does nothing.

`RESULT PASS` covers the speaker, the five servo columns, and output enable. It does not cover J2, J3, J4, the OLED, Wi-Fi, or a 2 A load. A fault in those can still be present.

No PCA answer prints `FAIL pca` and `RESULT FAIL pca`. The card says the PCA9685 did not answer and this board fails before the speaker test. There is no `r` / `c` / `a` for that failure. Unplug or `q`, same as any other verdict.

## Dialogue

The wire under each card is a short token. All accepted:

```
STEP tone
STEP servos
STEP oe
RESULT PASS
```

`tone` and `servos` rejected with `c`:

```
FAIL tone
FAIL servos
RESULT FAIL tone servos
```
