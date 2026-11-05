# Proposal

## Why

After `simplify-assembly-viewer`, dead API remains from removed animation loops and colour-picker/share plumbing. Stock paint and posing already use fixed defaults; leftover helpers and GLB extras only add confusion and tests for paths the pipeline never writes.

## What Changes

- Remove `blendPoses` from `poses.js` and its Node tests (animation interpolation gone; `REST` / `CENTRE` / `normalizePose` stay for posing and `check_poses`).
- Slim `colors.js`: drop `normalizeHex` and the `part.color` override branch; keep `GROUP_COLORS` / `PART_COLORS` / `HARDWARE_COLORS` / `FASTENER_FINISH` / `TRANSLUCENT` / `defaultColor` / `resolveColors`. Drop orphan `FASTENER_FINISH.name` if present.
- Strip unused colour/mod extras plumbing: `build_glb.mjs` no longer copies `entry.color` or `entry.mod`; viewer `main.js` no longer reads `extras.color` into part records.
- Update `test_viewer.mjs` (and any other callers) accordingly. No visual or UX change for the published viewer.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- (none — pure dead-code cleanup; `skip_specs: true`)

## Impact

- `web/js/assembly/poses.js`, `colors.js`, `main.js`
- `scripts/assembly/build_glb.mjs`, `test_viewer.mjs`
- No HTTP/firmware/CAD change; no docs rewrite unless a comment still describes colour overrides or pose blending
