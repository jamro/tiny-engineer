# Proposal

## Why

The assembly viewer still carries a full servo kinematics stack (pivot reparenting, pose math, sliders, and CI pose sweeps) after earlier cuts already made build steps the real product. Interactive joint posing is unused and keeps `layout()`, tests, and the GLB pipeline harder than they need to be. Dropping it end-to-end leaves a fixed assembled model plus build guidance.

## What Changes

- **BREAKING (viewer):** Remove per-joint pose sliders, the 90° reset control, and any runtime ability to rotate parts about servo shafts. The published model stays at its assembled (CAD / GLB) pose.
- **BREAKING (viewer API):** Drop `window.tinyEngineerAssembly.pose` / `applyPose` (and related pose state).
- Delete `web/js/assembly/joints.js` and `web/js/assembly/poses.js`.
- Delete `scripts/assembly/check_poses.mjs` and stop calling it from `scripts/assembly/build.sh`, docs, and CI/test lists.
- Slim `main.js` layout: parts stay under the CAD root (no pivot reparent); remove build-mode joint disable / force-centre logic.
- Remove joint chrome from `web/assembly/index.html` and related CSS; update copy that mentions posing joints.
- Trim pose-only assertions from `scripts/assembly/test_viewer.mjs` and `scripts/assembly/test_clipping.mjs`.
- Update `docs/3d/assembly-viewer.md`, `docs/testing.md`, and any AGENTS/testing mentions of pose checks.

**Out of scope (kept):** Fusion joint agreement in extract, arm rest-down bake in the GLB, build steps, step clipping (`check_clipping`), fasteners, boards, SG90-only pipeline, orbit/zoom Reset view, `#s=` step deep links.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `assembly-viewer`: Replace the “Manual joint posing” requirement with fixed assembled pose (no joint controls / no runtime kinematics). Drop pose-sweep language from the SG90 pipeline requirement so CI no longer requires `check_poses`.

## Impact

- **Web:** `web/js/assembly/main.js`, delete `joints.js` / `poses.js`; `web/assembly/index.html`; `web/css/base.css` (`.asm-joint*` rules).
- **Scripts / CI:** `scripts/assembly/build.sh`, delete `check_poses.mjs`; update `test_viewer.mjs` / `test_clipping.mjs`; docs that list `check_poses`.
- **Users:** Assembly page no longer poses servos; build walkthrough unchanged. Offline motion-range clip QA goes away (assembled + step clipping remain).
