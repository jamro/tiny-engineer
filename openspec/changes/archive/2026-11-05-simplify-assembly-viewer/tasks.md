# Tasks

## 1. Pipeline: SG90-only build

- [x] 1.1 Slim `scripts/assembly/build.sh` to extract SG90 → fasteners → check_steps → check_clipping → slim check_poses → build_glb only; remove preset and mod loops — verify script has no `presets.py` / `merge_mod` / `check_contacts` calls
- [x] 1.2 Delete `presets.py`, `test_presets.py`, `check_contacts.mjs`, `merge_mod.mjs`, and `web/assembly/mods/` (including halloween.json) — verify paths absent (`test ! -e …`)
- [x] 1.3 Delete `scripts/stage_print_files.mjs`; remove Pages print-stage step and unused path filters for print staging; drop `web/print/` from `.gitignore` if unused — verify `pages.yml` has no `stage_print_files` step
- [x] 1.4 Update `.github/workflows/ci.yml` assembly job to drop `test_presets.py` and any mod/preset-only commands; keep remaining SG90 tests — verify job script list matches kept tests
- [x] 1.5 Slim `check_poses.mjs` to centre/rest and joint-range sweep only (no animation / ring-hit loops); update callers — verify `node scripts/assembly/check_poses.mjs --help` / dry run mentions no animation flags or animation paths removed from source
- [x] 1.6 Rewrite or trim `test_build_glb.mjs` / `test_viewer.mjs` (and any other tests) so they no longer import deleted modules or assert multi-preset/mod/print ZIP behaviour — verify `node scripts/assembly/test_build_glb.mjs` and `node scripts/assembly/test_viewer.mjs` pass

## 2. Viewer UI and modules

- [x] 2.1 Strip `web/assembly/index.html`: remove Model/Servo switches, Print ZIP block, explode control, colour tab/panel, anim select/play; keep build UI, joint sliders/range, Electronics/Screws toggles — verify markup has no `asm-download`, `asm-explode`, `asm-anim`, `asm-tab-colors`, halloween/preset links
- [x] 2.2 Slim `web/js/assembly/main.js` to load only `models/sg90.glb`, apply default colours, build steps, joint sliders, visibility toggles; remove explode, colour share/storage, download, anim play, preset/mod hash handling — verify no imports from `download.js` and no `ANIMATIONS` / explode usage
- [x] 2.3 Slim `colors.js` to defaults-only (`defaultColor` / `resolveColors`); remove encode/decode/palette/finishes share helpers if unused — verify module exports used only for static paint
- [x] 2.4 Delete `web/js/assembly/download.js`; strip `poses.js` of `ANIMATIONS` and ring-hit/preset tables kept only for loops — verify files gone / no leftover exports referenced
- [x] 2.5 Remove unused assembly CSS for deleted controls (colour panel, explode, download, model switches) from `web/css/base.css` where safe — verify no selectors reference removed element ids

## 3. Docs and cross-links

- [x] 3.1 Rewrite `docs/3d/assembly-viewer.md` for SG90-only, no mods/ZIP/colours/explode/anims; update mermaid and commands — verify doc matches `build.sh` and viewer surface
- [x] 3.2 Update `docs/testing.md`, `AGENTS.md`, `docs/3d/order-parts.md`, `mods/README.md`, `mods/halloween/README.md`, and any other links that advertised viewer presets/mods/ZIP — verify no `#m=halloween` / Print ZIP / multi-preset viewer claims remain

## 4. Integration check

- [x] 4.1 Run SG90 assembly build locally (`scripts/assembly/build.sh` into a temp or `web/assembly/models`) and confirm `sg90.glb` exists and checks pass
- [x] 4.2 Smoke the viewer (static server on `web/`) — build steps and joint sliders work; no download/colour/explode/anim/preset/mod UI — verify visually or via DOM query that removed controls are absent

## 5. Viewer polish

- [x] 5.1 Remove Electronics and Screws toggles from `web/assembly/index.html` and `main.js` layout; always show hardware/fasteners when the step allows — verify no `asm-electronics` / `asm-fasteners` and both kinds visible on the finished robot
- [x] 5.2 Stop writing the inventory status string on ready (printed / electronics / screws · SG90); keep loading and error status only — verify ready state has no inventory summary
- [x] 5.3 Remove the joint range `<select>`; always use `DEFAULT_RANGE` (25°) for sliders and 90° reset — verify no `asm-range` control and poses use ±25°
- [x] 5.4 Remove the pose-scale hint under the joint sliders — verify that copy is gone from markup
- [x] 5.5 Move Back / Play / Next to the upper part of the build player (above variable step body); adjust CSS so nav does not jump with text length — verify nav precedes step title/text in the DOM and stays visually top-stable
- [x] 5.6 Update `docs/3d/assembly-viewer.md` (and CSS) for always-visible electronics/screws, fixed ±25°, no inventory status / pose hint — verify docs match the UI
- [x] 5.7 Smoke DOM/source check: absent toggles/range/hint/inventory; present top step nav — verify with a script or manual pass

## Workflow follow-up

- After section 5 is applied, re-smoke the viewer, then archive (`/opsx-archive`).
- Confirm Pages deploy publishes only `models/sg90.glb`.
