# Tests

Host checks. These commands do not flash the board. GitHub Actions on `main` and PRs runs these same commands (firmware, packages, and PCB). On-device hardware: [hardware/testing.md](hardware/testing.md).

## Firmware

Unity on the host. Compiles `src/settings/validate.cpp` plus header-only helpers such as [`include/servos.h`](../include/servos.h).

```bash
pio test -e native
```

`pio run` builds firmware. Native tests are **`pio test -e native`**, not `pio run -e native`.

## OLED expressions

The optional [expression library](../lib/TinyEngineerExpressions/README.md) has deterministic asset checks and native renderer tests. These checks require Node 18+ and PlatformIO; they do not access hardware. On the robot, enable those faces with Config / `eyes_style=kaomoji` (default remains `classic`).

```bash
node scripts/expressions/generate.js --check
node scripts/expressions/test-assets.js
pio test -e native
pio run -e expression-demo
```

The `expression-demo` environment builds a separate OLED-only application without changing the default robot build. For a later bench test, follow its [wiring and upload notes](../lib/TinyEngineerExpressions/README.md#standalone-oled-demo), leave the separate servo supply off, and observe all 16 faces through a complete 48-second cycle. Report physical display results separately from host tests and compilation.

## Packages

Node 18+. No robot.

```bash
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