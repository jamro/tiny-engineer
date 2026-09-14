# Contributing to Tiny Engineer

Issues and PRs welcome — especially new agent integrations. Search existing issues first. One concern per PR. Open an issue before a large CAD or HTTP API change.

This is a desk robot: firmware on an ESP32-C3, 3D-printed mechanics, and HTTP clients that drive poses. Contribution rules follow that split.

## Ways to help

- Firmware (`src/`, `include/`, `lib/`, `data/`)
- Integrations: Cursor hooks, Antigravity CLI, or any REST client ([docs/integration.md](docs/integration.md))
- Docs, wiring, BOM corrections
- CAD / printables (`3d_models/`)
- KiCad PCB boards (`hardware/boards/` — [docs/pcb.md](docs/pcb.md))
- Photos of a working build or a failure (brownout, binding, blink codes)

## Legal

Opening a PR licenses your change under the license of the files you touch. No CLA or DCO.

| What | License |
| --- | --- |
| Firmware, packages, scripts, docs | [MIT](LICENSE) — see [LICENSING.md](LICENSING.md) |
| `3d_models/cad/`, `3d_models/parts/` | [CERN-OHL-S-2.0](3d_models/LICENSE) |
| `hardware/boards/` | [CERN-OHL-S-2.0](hardware/LICENSE) |

If you modify the hardware designs and distribute Products based on them, CERN-OHL-S-2.0 requires you to make the Complete Source available under the same license. Keep [3d_models/NOTICE](3d_models/NOTICE) and [hardware/NOTICE](hardware/NOTICE) Source Location accurate for the revision you ship.

The **Tiny Engineer** name, logo, and [`3d_models/parts/AiEmblem.3mf`](3d_models/parts/AiEmblem.3mf) are **not** licensed. Factual “based on Tiny Engineer” is fine. Do not imply an official product. Details: [TRADEMARK.md](TRADEMARK.md).

New hardware paths need a matching `[[annotations]]` block in [REUSE.toml](REUSE.toml).

## Setup and checks

PlatformIO from the repo root. Node 18+ for packages. GitHub Actions on `main` and PRs runs the same commands ([.github/workflows/ci.yml](.github/workflows/ci.yml)). Host only — CI never flashes.

```bash
pio run
pio test -e native
npm test --prefix packages/tiny-engineer-cursor
npm test --prefix packages/tiny-engineer-antigravity
```

`pio test -e native` is not `pio run -e native`. Details: [docs/testing.md](docs/testing.md). Flash locally with `pio run -t upload` when you have a board. Physical module is a **Waveshare ESP32-C3-Zero**; PlatformIO `board = esp32-c3-devkitm-1` is the build target name.

## By change type

**Firmware.** `pio run` plus native tests when you touch settings validation. Pin changes update [`include/pins.h`](include/pins.h) **and** [`docs/hardware/`](docs/hardware/) in the same PR.

**HTTP / API.** Same PR must update the HTML index ([`src/http/index_page.cpp`](src/http/index_page.cpp)) and [`docs/api.md`](docs/api.md). If the route list, params, or boot URLs changed, also update [`README.md`](README.md) and [`docs/hardware/testing.md`](docs/hardware/testing.md). See [.cursor/rules/sync-api-endpoints.mdc](.cursor/rules/sync-api-endpoints.mdc).

**Settings.** Follow the layer checklist in [docs/settings.md](docs/settings.md). Never log the raw `access_token` (set/unset only).

**Integrations.** Add or extend tests in the package you change. Raw REST examples belong in [docs/integration.md](docs/integration.md). Prefer short timeouts and ignore network errors so a missing robot does not stall the agent.

**CAD.** Edit [`3d_models/cad/TinyEngineer.f3d`](3d_models/cad/TinyEngineer.f3d) **and** export the affected [`3d_models/parts/{servo_id}/3mf/*.3mf`](3d_models/parts/). Keep CERN-OHL-S. Do not swap `AiEmblem.3mf` as a branding change.

**PCB.** Follow the [PCB checklist](docs/pcb.md#checklist). One board per `hardware/boards/<name>/`, KiCad 10, ERC and DRC reviewed, no generated Gerbers or other fab outputs. Keep CERN-OHL-S. New board paths need a matching `[[annotations]]` block in [REUSE.toml](REUSE.toml).

**Motion.** Animations use −1..1 poses mapped to the saved min/max in [docs/robot-movement.md](docs/robot-movement.md). Stock defaults live in [`include/servos.h`](include/servos.h). Do not widen NVS servo clamps without testing on a real robot. Setup AP `POST /setup/servo` can use 0–180° to find limits; assembled motion must not.

## Bench safety

- Supply **5 V / ≥ 2 A**. Five PowerHD HD-1370A stalls are ~1.3–1.6 A before Wi-Fi and audio; SG90 (recommended) typically stalls harder ([docs/hardware/power.md](docs/hardware/power.md)).
- Never power servos from the ESP32 3.3 V LDO.
- PCA9685 **VCC** = 3.3 V logic; **V+** = 5 V servo rail. Do not short them.
- Leave the C3-Zero ceramic antenna clear of metal and dense plastic.

Say in the PR whether you tested on a robot, on the bench, or with no hardware.

## Commits (Conventional Commits → SemVer)

Format: `type(scope): summary`

- Imperative mood, lowercase `type`, no trailing period
- One scope from the list below, or omit scope if the change spans several
- PR title uses the same format (squash-friendly)

| Type | SemVer |
| --- | --- |
| `feat` | MINOR |
| `fix` | PATCH |
| `feat!:` / `fix!:` or footer `BREAKING CHANGE:` | MAJOR |
| `docs`, `style`, `test`, `chore`, `ci`, `refactor` (unless breaking) | no bump |

Scopes: `firmware`, `http`, `settings`, `anim`, `servos`, `wifi`, `cursor`, `antigravity`, `cad`, `pcb`, `docs`, `scripts`, `ci`.

**Breaking in this repo** means: removed or renamed HTTP route or query param; NVS key rename that drops existing settings; pinout change; default servo range change that invalidates calibration; hook CLI flag or event rename. Call it out with `!` on the type and a `BREAKING CHANGE:` footer.

CAD-only `feat(cad)` / `fix(cad)` and PCB-only `feat(pcb)` / `fix(pcb)` version the hardware design, not the npm packages.

Examples:

```
feat(anim): add dead pose
fix(cad): add screw hole to desk pad
feat(pcb): add controller board
feat(http)!: drop query alias on /anim

BREAKING CHANGE: POST /anim no longer accepts the old `n` alias; use `name`.
```

## Pull requests

- CI green for the area you touched
- Docs (and HTML index, if HTTP) in the same change
- Photos for wiring or CAD
- No `.env`, tokens, or Wi-Fi passwords in logs or screenshots
- Commit and PR title match `type(scope): summary`

Vulnerabilities: [SECURITY.md](SECURITY.md), not a public issue. Conduct: [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
