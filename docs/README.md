# Docs index

Pick a path. Depth lives in the linked pages.

### Building the robot

| You want to… | Start here |
| --- | --- |
| **Build from scratch** (canonical checklist) | [getting-started.md](getting-started.md) |
| **Buy parts** (main control board + cart) | [shopping.md](shopping.md) |
| **Bridge into hardware/electronics** (software engineer background) | [hardware-for-software-engineers/README.md](hardware-for-software-engineers/README.md) — optional parallel reading; safety card lives on the checklist |
| **Assemble printed parts** (mechanical procedure) | [3d/assembly.md](3d/assembly.md) |
| **Flash firmware** | [flash.md](flash.md) |
| **Electronics** (main control board) | [shopping.md](shopping.md) → [hardware/main-control-board.md](hardware/main-control-board.md) → [hardware/README.md](hardware/README.md) |
| **Extra modules** (advanced breakouts) | [shopping.md](shopping.md#advanced-extra-modules) → [hardware/wiring.md](hardware/wiring.md) → [3d/assembly-modular.md](3d/assembly-modular.md) |
| **Print / parts inventory** | [../3d_models/README.md](../3d_models/README.md) |
| **Order printed parts** (no 3D printer) | [3d/order-parts.md](3d/order-parts.md) |
| **Order main control board** (PCB / PCBA) | [hardware/order-main-control-board.md](hardware/order-main-control-board.md) |
| **Adapt CAD to a different servo** | [3d/parametric-design.md](3d/parametric-design.md) |
| **Add a new CAD / printable part** | [3d/adding-parts.md](3d/adding-parts.md) |
| **Optional mods** | [../mods/README.md](../mods/README.md) |
| **Use with an agent** (robot already on Wi-Fi) | [integration.md](integration.md) · Cursor: [hooks.md](hooks.md) |

### Reference and contribute

| You want to… | Start here |
| --- | --- |
| **Contribute a PCB** (KiCad) | [pcb.md](pcb.md) → [`hardware/`](../hardware/README.md) |
| **HTTP API / settings** | [api.md](api.md); add a setting: [settings.md](settings.md) |
| **Any IDE / scripts** | [integration.md](integration.md) |
| **Firmware / package tests** | [testing.md](testing.md) |
| **AI coding agents (repo rules)** | [AGENTS.md](../AGENTS.md) |
| **Contribute / report a vuln** | [CONTRIBUTING.md](../CONTRIBUTING.md); [SECURITY.md](../SECURITY.md) |
| **Share a finished build** | [Show and tell](https://github.com/jamro/tiny-engineer/discussions/categories/show-and-tell) — [how to post](../CONTRIBUTING.md#share-your-build) |

Also: [robot-movement.md](robot-movement.md) (servo axes and safe ranges); optional [macos-lock-unlock.md](macos-lock-unlock.md) (sleep/wake on Mac screen lock).

**Source of truth:** pin constants in firmware (`include/pins.h`); electrical detail in `docs/hardware/`; HTTP shapes in [api.md](api.md). Overall build order: [getting-started.md](getting-started.md).
