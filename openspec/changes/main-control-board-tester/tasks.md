# Tasks

## 1. PCBA test image

- [x] 1.1 Add `[env:pcba-test]` and `examples/pcba-test/main.cpp` that prints `PCBA-TEST v1` on USB CDC and does not attach product `extra_scripts`. Verify `pio run -e pcba-test` exits 0 and `default_envs` in `platformio.ini` is still `esp32-c3-devkitm-1`.
- [x] 1.2 Implement the session in that `main`: PCA probe at `0x40`, steps `tone`, `head`, `head-off`, `neck`, `arm-l`, `arm-r`, `body`, keys Enter/`n`/`r`/`c`/`a`/`q`, repeating tone until Enter or `n`, OE disabled only during `head-off` and enabled again on Enter, `c`, or `a`, `n` after `RESULT` ignored, and lines `PCBA-TEST v1`, `STEP`, `FAIL`, `RESULT PASS`, `RESULT FAIL` with space-separated names, and `STOP`. Verify `pio run -e pcba-test` exits 0 and the example source names those steps in that order.
- [x] 1.3 Hold the onboard LED on GPIO10 steady white with equal channels from boot through STOP, including a PCA failure. Do not blink it and do not read `rgb_order`. Verify `pio run -e pcba-test` exits 0 and the example source writes equal channel values to that pin.

## 2. Host and operator card

- [x] 2.1 Add `scripts/pcba_test.py` that reads `STEP`, `FAIL`, `RESULT`, and `STOP` lines, sends Enter as a line feed plus `n`, `r`, `c`, `a`, and `q`, shows `r`, `c`, and `a` when the line is `FAIL`, and on start prints `https://tinyengineer.dev/pcba_test` as the alternate tester. Verify a unit test feeds a scripted accept path and a scripted `n` then `c` path, checks the shown step and the `r`/`c`/`a` prompt, and checks that startup text contains that URL.
- [x] 2.2 On port loss, wait and open the returning CDC port. Verify a unit test drops the fake port, reconnects, and checks the next session banner.
- [x] 2.3 Write `docs/hardware/pcba-test.md` as the procedure card: bench setup, plug facing, plug stays on HEAD through `head-off`, lamp pattern, J1, keys, unplug for the next board, golden module stays with the tester, `python3 scripts/pcba_test.py`, and `https://tinyengineer.dev/pcba_test`. Verify the doc lists `tone`, `head`, `head-off`, `neck`, `arm-l`, `arm-r`, `body`, keys `r`, `c`, `a`, `n`, `q`, `python3 scripts/pcba_test.py`, `https://tinyengineer.dev/pcba_test`, plug stays on HEAD, red on and green off, square pad as GND, J1, and that the golden module stays with the tester.
- [x] 2.4 Add `web/pcba_test/index.html` that, when `navigator.serial` is missing or the context is not secure, says this browser cannot run the test, names `python3 scripts/pcba_test.py`, and shows no connect control. Verify a unit test of that gate for the failing cases and the Chrome-on-HTTPS case, and that no other HTML file under `web/` links to `pcba_test`.
- [x] 2.5 On a passing browser, open the golden module with Web Serial, show the current step, forward Enter, `n`, `r`, `c`, `a`, and `q`, and reopen the granted port when it returns without reloading the page. Verify a unit test feeds a scripted `STEP` then `RESULT` and a disconnect-then-connect, and checks the shown step and that the page did not ask for a new grant.
- [x] 2.6 Update `docs/hardware/pcba-test.md` so the bench setup says the module LED stays steady white while the test image has power, and that this lamp is not the board verdict. Verify the doc contains `steady white` and still lists the plug looks `red on and green off`.

## 3. Session dry run

- [x] 3.1 Point the host at a fake device that prints the banner and walks an all-accept session, then a session that fails `tone` and `body` with `c`. Verify the host log shows `RESULT PASS` and `RESULT FAIL` with those two names, and the operator card matches that dialogue.

## 4. On-screen coaching

- [x] 4.1 In both hosts, show English coaching for every `STEP`: the index `1/7` through `7/7`, what is tested, what to do, the expected effect, `Enter = good`, `n = bad`, and the wire token. The script and the page use the same seven sentences. Verify unit tests for `STEP tone` (`1/7`, speaker on J1, plug stays on HEAD, repeating tone), `STEP head-off` (`3/7`, plug stays on HEAD, red on and green off), and `STEP neck` (`4/7`, move the plug, red on and green on).
- [x] 4.2 On a step `FAIL`, show that step's expected effect and `r = try this step again`, `c = keep the fail, go on`, `a = start this board over`. On `FAIL pca`, show that the PCA9685 did not answer and do not show `r`, `c`, or `a`. On the banner, say the module LED stays steady white and is not the board verdict. Verify unit tests for `FAIL head`, `FAIL pca`, and `PCBA-TEST v1`.
