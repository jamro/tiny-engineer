# Tests

Host checks. Neither flashes the board. GitHub Actions on `main` and PRs runs these same commands (firmware, packages, and PCB). On-device hardware: [hardware/testing.md](hardware/testing.md).

## Firmware

Unity on the host. Compiles `src/settings/validate.cpp` plus header-only helpers such as [`include/servos.h`](../include/servos.h).

```bash
pio test -e native
```

`pio run` builds firmware. Native tests are **`pio test -e native`**, not `pio run -e native`.

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