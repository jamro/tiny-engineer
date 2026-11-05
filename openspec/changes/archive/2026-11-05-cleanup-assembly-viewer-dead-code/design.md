# Design

## Context

See proposal.md — Why. After simplify, `poses.js` still exports `blendPoses` (only tested, never called). `colors.js` still accepts per-part hex via `normalizeHex(part.color)` though stock `assembly.json` never sets `color`. `build_glb.mjs` still forwards `entry.color` / `entry.mod` into GLB extras; viewer reads `extras.color`. Paint truth today is print-group / part-id / hardware-component tables plus fixed fastener finish and vertex colours on boards.

## Goals / Non-Goals

**Goals:**
- Delete unreachable pose-blend API and its tests.
- Make colour resolution table-only (no hex override path, no GLB colour/mod extras).
- Keep published viewer appearance and joint/check behaviour identical.

**Non-Goals:**
- Removing Play autoplay, screw slide phases, or clipping/pose checks.
- Merging `colors.js` into `main.js`.
- Changing default hex values or fastener metalness/roughness.
- Docs rewrite beyond fixing comments that describe removed APIs.

## Decisions

1. **Drop `blendPoses` only; keep `REST`.**  
   `check_poses.mjs` still samples the rest pose. Sliders still use `normalizePose`.  
   Alternative: inline rest samples in the check script — rejected; shared constant stays one source of truth.

2. **Colour cleanup depth = explore option B.**  
   Remove `normalizeHex` + `part.color` branch + XSS-style hex tests; remove `extras.color` / `entry.mod` plumbing in GLB and `main.js`; drop `FASTENER_FINISH.name` if unused. Keep colour tables and `resolveColors`.  
   Alternative A (hex-only): leaves dead GLB fields. Alternative C (delete `colors.js`): hurts Node unit tests — rejected.

3. **`skip_specs: true`.**  
   No observable requirement change vs `assembly-viewer` (defaults only, no picker). Avoid inventing a MODIFIED requirement for an internal paint path.

## Risks / Trade-offs

- **[Risk]** Future desire to tint a single part via `assembly.json` `color` → Mitigation: add that field back deliberately; today pipeline never writes it.
- **[Risk]** External/custom GLB with `extras.color` ignored → Mitigation: only stock `models/sg90.glb` is supported; custom GLBs already out of scope.
- **[Trade-off]** Slightly less defensive hex parsing vs less code — accept; no user-supplied colour strings remain.

## Migration Plan

Land as a normal code change on the assembly branch. No GLB rebuild required for correctness (paint tables unchanged), but regenerating `sg90.glb` optionally drops unused extras keys on next CI cache miss. Rollback = revert commit.
