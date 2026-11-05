# Tasks

## 1. Pipeline and kinematics modules

- [x] 1.1 Remove `check_poses.mjs` from `scripts/assembly/build.sh` and delete `scripts/assembly/check_poses.mjs` — verify `build.sh` has no `check_poses` reference and the file is gone
- [x] 1.2 Delete `web/js/assembly/joints.js` and `web/js/assembly/poses.js` — verify nothing under `web/` or `scripts/assembly/` still imports them (`rg joints\\.js|poses\\.js|check_poses`)

## 2. Viewer UI and runtime

- [x] 2.1 Strip joint chrome from `web/assembly/index.html` (`#asm-joints`, sliders, 90° button) and update meta/intro copy that mentions posing joints — verify markup has no `asm-joint` / `asm-pose` ids
- [x] 2.2 Remove `.asm-joint*` CSS from `web/css/base.css` — verify no remaining `.asm-joint` rules
- [x] 2.3 Slim `web/js/assembly/main.js`: drop `rig` / pose imports, sliders, `applyPose`, build-mode joint disable, and `window.tinyEngineerAssembly.pose` / `applyPose`; simplify `layout()` to root-local offsets only — verify `main.js` has no `joints`/`poses` imports and no `parent !== root` offset path

## 3. Tests and docs

- [x] 3.1 Remove pose / `withFollowers` assertions from `scripts/assembly/test_viewer.mjs` and drop `checkPoses` / `poser` usage from `scripts/assembly/test_clipping.mjs` — verify `node scripts/assembly/test_viewer.mjs` and `node scripts/assembly/test_clipping.mjs` pass
- [x] 3.2 Update `docs/3d/assembly-viewer.md` (pipeline diagram, Move / check_poses sections) and remove `check_poses` from `docs/testing.md` (and any other live docs that still name it) — verify docs no longer reference `check_poses`, `joints.js`, or joint sliders as viewer features
- [x] 3.3 Update `openspec/specs/assembly-viewer/spec.md` Purpose to drop “manual joint posing” (fixed assembled pose / build guidance) when applying this delta — verify Purpose matches the new ADDED/REMOVED requirements

## 4. Integration check

- [x] 4.1 Smoke the assembly page (static server on `web/`): build steps still work; no joint controls; finished robot at assembled pose — verify DOM has no `#asm-joints` and build Back/Play/Next still operate

## Workflow follow-up

- Archive the change after review (`/opsx-archive` or project archive flow) so the main `assembly-viewer` spec picks up Fixed assembled pose and drops Manual joint posing.
