# Tests

Host checks. These commands do not flash the board. GitHub Actions on `main` and PRs runs these same commands (firmware, packages, and PCB). On-device hardware: [hardware/testing.md](hardware/testing.md).

## Firmware

Unity on the host. Compiles `src/settings/validate.cpp` plus header-only helpers such as [`include/servos.h`](../include/servos.h).

```bash
pio test -e native
python3 scripts/test_audio_pack.py
```

`pio run` builds firmware. Native tests are **`pio test -e native`**, not `pio run -e native`. `scripts/test_audio_pack.py` checks the stock WAV copy and mod overlay (no PlatformIO, no hardware). On GitHub Actions the firmware job also runs `pio run -t buildfs` and uploads `bootloader.bin`, `partitions.bin`, `firmware.bin`, and `littlefs.bin` as the `firmware-<sha>` artifact.

## OLED expressions

The optional [expression library](../lib/TinyEngineerExpressions/README.md) has deterministic asset checks and native renderer tests. These checks require Node 18+ and PlatformIO; they do not access hardware. On the robot, enable those faces with Config / `eyes_style=kaomoji` (default remains `classic`).

```bash
node scripts/expressions/generate.js --check
node scripts/expressions/test-assets.js
pio test -e native
pio run -e expression-demo
```

The `expression-demo` environment builds a separate OLED-only application without changing the default robot build. For a later bench test, follow its [wiring and upload notes](../lib/TinyEngineerExpressions/README.md#standalone-oled-demo), leave the separate servo supply off, and observe all 16 faces through a complete 48-second cycle. Report physical display results separately from host tests and compilation.

## Web UI

The control panel in [`ui/`](../ui/) is linted with [html-validate](https://html-validate.org/), ESLint, [Stylelint](https://stylelint.io/) (`stylelint-config-standard`) and Prettier. Node 20.19+ on 20.x, or 22.12+.

```bash
npm ci --prefix ui
npm run lint --prefix ui
npm test --prefix ui
npm run format --prefix ui   # apply Prettier
```

`npm test` runs [Vitest](https://vitest.dev/) in jsdom: each test loads the real `index.html`, imports the UI modules, and drives them against a fake robot that stubs `fetch` and records requests ([`ui/test/robot.js`](../ui/test/robot.js)). The tests cover the auth gate, config saving and access-token handling, the servo page range hint, and the setup wizard (step order, what each step saves, step-only calibration moves, LED order validation).

## Packages

Node 18+. No robot.

```bash
npm test --prefix packages/tiny-engineer-cli
npm test --prefix packages/tiny-engineer-cursor
npm test --prefix packages/tiny-engineer-antigravity
npm test --prefix packages/tiny-engineer-claude-code
```

## PCB

KiCad 10 (`kicad-cli`). Stdlib Python only — no pip packages. ERC (errors), DRC (errors + schematic parity), and `expected-nets.yml` for every board under `hardware/boards/`.

```bash
python3 scripts/check_pcb.py
```

Optional: `python3 scripts/check_pcb.py main-control-board --report-dir artifacts/pcb`. Details: [pcb.md](pcb.md). On-device hardware: [hardware/testing.md](hardware/testing.md).