# AGENTS.md

Tool-agnostic brief for coding agents working **on this repository**.

Driving the robot from an IDE or script is a separate concern — use [docs/integration.md](docs/integration.md) and [docs/api.md](docs/api.md) (`POST /anim`). This file is about changing firmware, packages, docs, CAD, and PCB.

## Project

Tiny Engineer is an open-source ESP32-C3 Wi-Fi desk robot: 3D-printed mechanics, REST-controlled animations, and Node packages that map agent hooks to poses.

| Path | Role |
| --- | --- |
| `src/`, `include/`, `lib/`, `data/` | Firmware (PlatformIO) |
| `packages/` | HTTP / hook CLIs (Cursor, Antigravity, …) — scope `integrations` |
| `3d_models/` | CAD and printables (CERN-OHL-S) |
| `hardware/` | KiCad boards (CERN-OHL-S) |
| `docs/` | Human docs; depth lives here |

Physical module: **Waveshare ESP32-C3-Zero**. PlatformIO `board = esp32-c3-devkitm-1` is the build target name.

## Commands

From the repo root (Node 18+ for packages). Host only — CI never flashes.

```bash
pio run
pio test -e native
npm test --prefix packages/tiny-engineer-cursor
npm test --prefix packages/tiny-engineer-antigravity
```

`pio test -e native` is not `pio run -e native`. Details: [docs/testing.md](docs/testing.md).

Do **not** flash (`pio run -t upload`) unless the user asks.

## After significant changes

Significant: logic, types, APIs, headers, firmware under `src/` / `include/` / `lib/`, or `platformio.ini`.

- Run `pio run` and fix failures before considering the work done.
- Skip compile for comments-only, docs-only, or tiny formatting.

## HTTP API sync

When adding, removing, or changing method, path, or query params of any HTTP route, update in the **same** change:

1. **HTML index** — [`src/http/index_page.cpp`](src/http/index_page.cpp): endpoint table and param lists for `/anim`, `/settings`, `/test/servo`
2. **API reference** — [`docs/api.md`](docs/api.md)
3. **Summaries** — if the endpoint list, params, or boot URLs changed: [`README.md`](README.md) and [`docs/hardware/testing.md`](docs/hardware/testing.md)
4. **405 paths** — if needed: `isHttpTestPath()` / `handleNotFound` in `src/http/`

HTML index: one-line route descriptions only (except supported-parameter lists). Param values must match `docs/api.md` exactly.

## Commits

Format: `type(scope): summary` (imperative, lowercase type, no trailing period; PR title same).

Scopes: `firmware`, `http`, `settings`, `anim`, `servos`, `wifi`, `integrations`, `cad`, `pcb`, `docs`, `scripts`, `ci`.  
`integrations` = anything under `packages/`. Do not add a new scope per package.

Breaking rules, SemVer mapping, and examples: [CONTRIBUTING.md](CONTRIBUTING.md).

## By change type (short)

- **Firmware** — `pio run`; also `pio test -e native` when touching settings validation. Pin changes update `include/pins.h` **and** `docs/hardware/` together.
- **Settings** — layer checklist in [docs/settings.md](docs/settings.md). Never log raw `access_token`.
- **Integrations** — add/extend package tests; prefer short timeouts and ignore network errors so a missing robot does not stall the agent.
- **CAD** — edit `.f3d` **and** export affected `3mf`. CERN-OHL-S. Do not swap `AiEmblem.3mf` as branding.
- **PCB** — [docs/pcb.md](docs/pcb.md) checklist + KiCad review rules below. New board paths need `REUSE.toml`. CERN-OHL-S.
- **Motion** — poses −1..1 mapped to saved min/max; see [docs/robot-movement.md](docs/robot-movement.md). Do not widen NVS servo clamps without testing on a real robot.
- **Secrets** — no `.env`, tokens, or Wi-Fi passwords in logs or screenshots.

## KiCad / PCB review

Truth order (do not skip ahead for connectivity claims):

1. `kicad-cli` netlist export
2. ERC / DRC
3. board `expected-nets.yml` (compare to netlist)
4. [`docs/hardware/pinout.md`](docs/hardware/pinout.md) / [`include/pins.h`](include/pins.h)
5. board `README.md`

**Forbidden:** infer nets from `.kicad_sch` / `.kicad_pcb` X/Y, geometry, or coordinate heuristics.

**Required:** netlist ↔ `expected-nets.yml` comparison for connectivity. No `kicad-cli` → say so and **skip** connectivity claims (layout/docs review only).

Contribute/review process: [docs/pcb.md](docs/pcb.md). `expected-nets.yml`: [docs/pcb.md#expected-netsyml](docs/pcb.md#expected-netsyml).

## Bench safety

- Supply **5 V / ≥ 2 A**. Never power servos from the ESP32 3.3 V LDO.
- PCA9685 **VCC** = 3.3 V logic; **V+** = 5 V servo rail. Do not short them.
- Leave the C3-Zero ceramic antenna clear of metal and dense plastic.

## Read next

| Topic | Doc |
| --- | --- |
| HTTP API | [docs/api.md](docs/api.md) |
| Drive robot from any IDE | [docs/integration.md](docs/integration.md) |
| Settings layers | [docs/settings.md](docs/settings.md) |
| Servo axes / ranges | [docs/robot-movement.md](docs/robot-movement.md) |
| Tests | [docs/testing.md](docs/testing.md) |
| PCB / KiCad | [docs/pcb.md](docs/pcb.md) |
| Humans contributing | [CONTRIBUTING.md](CONTRIBUTING.md) |
| Docs index | [docs/README.md](docs/README.md) |
