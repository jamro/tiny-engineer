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
pio run -e pcba-test
python3 scripts/test_pcba_test.py
node scripts/test_pcba_web.mjs
```

The `expression-demo` environment builds a separate OLED-only application without changing the default robot build. `pcba-test` builds the PCBA session the same way. Bench steps: [hardware/pcba-test.md](hardware/pcba-test.md). For a later expression-demo bench test, follow the [wiring and upload notes](../lib/TinyEngineerExpressions/README.md#standalone-oled-demo), leave the separate servo supply off, and observe all 16 faces through a complete 48-second cycle. Report physical display results separately from host tests and compilation.

## Web UI

The control panel in [`ui/`](../ui/) (TypeScript and Preact) is type-checked with `tsc` and linted with ESLint, [Stylelint](https://stylelint.io/) (`stylelint-config-standard`) and Prettier. Node 20.19+ on 20.x, or 22.12+.

```bash
npm ci --prefix ui
npm run lint --prefix ui
npm test --prefix ui
npm run format --prefix ui   # apply Prettier
```

`npm test` runs [Vitest](https://vitest.dev/) in jsdom: each test renders the app with [Testing Library](https://testing-library.com/docs/preact-testing-library/intro/) and drives it against a fake robot that stubs `fetch` and records requests ([`ui/test/robot.tsx`](../ui/test/robot.tsx)). The tests cover the auth gate, config saving and access-token handling, the staged WiFi network list, the servo page range hint, and the setup wizard (servos hub, skip/assembly/guided calibration, what each step saves, LED order validation).

## Packages

Node 18+. No robot.

```bash
npm test --prefix packages/tiny-engineer-cli
npm test --prefix packages/tiny-engineer-cursor
npm test --prefix packages/tiny-engineer-antigravity
npm test --prefix packages/tiny-engineer-claude-code
```

## 3D assembly model

Python 3.13 with [`scripts/assembly/requirements.txt`](../scripts/assembly/requirements.txt), and Node 18+. Reads the Fusion design, checks every part's placement against its exported STL, and packs the GLB. Then checks the viewer's build steps on the meshes: no part may pass through another while it moves in, and no two parts may sit inside each other when assembled. Then checks the joint poses and animation loops the same way, and that the bell-ringing strike lands on the bell. CI job `assembly`. Details: [3d/assembly-viewer.md](3d/assembly-viewer.md#clipping-check), [pose check](3d/assembly-viewer.md#pose-check).

```bash
python3 scripts/assembly/test_assembly.py
node scripts/assembly/test_build_glb.mjs
node scripts/assembly/test_viewer.mjs
node scripts/assembly/test_clipping.mjs
python3 scripts/assembly/test_presets.py
python3 scripts/assembly/extract.py --preset sg90 --out artifacts/assembly/sg90
node scripts/assembly/build_glb.mjs artifacts/assembly/sg90/assembly.json artifacts/assembly/sg90.glb
node scripts/assembly/check_clipping.mjs artifacts/assembly/sg90/assembly.json web/assembly/steps.json
node scripts/assembly/check_poses.mjs artifacts/assembly/sg90/assembly.json web/assembly/steps.json
```

`scripts/assembly/build.sh` runs the extraction, both step checks and the GLB build for stock, every other servo preset (`presets.py`, plus `check_contacts.mjs`) and every mod, and the pose check for stock and every preset.

## PCB

KiCad 10 (`kicad-cli`). Stdlib Python only — no pip packages. ERC (errors), DRC (errors + schematic parity), and `expected-nets.yml` for every board under `hardware/boards/`.

```bash
python3 scripts/check_pcb.py
```

Optional: `python3 scripts/check_pcb.py main-control-board --report-dir artifacts/pcb`. Details: [pcb.md](pcb.md). On-device hardware: [hardware/testing.md](hardware/testing.md).