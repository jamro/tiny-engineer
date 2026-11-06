# Design

## Context

See proposal.md — Why. Current viewer loads `models/sg90.glb`, paints defaults, runs build steps, and also `rig()`s five servo pivots from `JOINTS` in `poses.js`, with sliders driving `applyPose`. `layout()` must re-express build-step offsets in each part's parent frame because `pivot.attach` reparents meshes under joint groups. CI runs `check_poses.mjs` after step/clipping checks; that script duplicates viewer kinematics offline.

Fork B (confirmed): delete the kinematics stack end-to-end, not only the UI.

## Goals / Non-Goals

**Goals:**
- Fixed assembled pose in the browser; no joint chrome or runtime pivots.
- Delete `joints.js`, `poses.js`, `check_poses.mjs` and all call sites / tests that exist only for them.
- Simplify `layout()` to root-local offsets (parts stay children of the CAD root).
- Docs and AGENTS/testing command lists match the shorter pipeline: extract → fasteners → check_steps → check_clipping → build_glb.

**Non-Goals:**
- Changing Fusion joint agreement, arm rest-down bake, fasteners, boards, or step clipping.
- Replacing Three.js or redesigning build-step UX.
- Adding a stub/feature flag to re-enable posing later.

## Decisions

1. **Hard-delete kinematics, not feature-flag**  
   Remove modules and UI rather than hide `#asm-joints`. Dead pose code would re-grow.  
   *Alt:* keep `poses.js` / `check_poses` for CAD QA without UI — rejected (fork A); user chose B.

2. **Parts stay under CAD root**  
   Without `rig()`, `layout()` drops the `parent !== root` quaternion path; step offsets apply in root frame only.  
   *Alt:* keep empty pivot groups — rejected; no benefit.

3. **Drop debug pose API with the feature**  
   Remove `pose` / `applyPose` from `window.tinyEngineerAssembly`.  
   *Alt:* no-op stubs — rejected; lying API.

4. **Update capability Purpose on apply/archive**  
   Main `openspec/specs/assembly-viewer/spec.md` Purpose still says “manual joint posing”; rewrite to build guidance at fixed assembled pose when syncing the delta (same change as applying REMOVED/ADDED/MODIFIED requirements).

5. **Docs follow code in the same change**  
   `docs/3d/assembly-viewer.md` diagram and Move section; `docs/testing.md` / AGENTS command lists that name `check_poses`.

## Risks / Trade-offs

- **[Risk] CAD change introduces motion-range clip (e.g. rest vs keyboard) with no CI catch** → Mitigation: assembled + step `check_clipping` remain; physical mount guide and extract rest-down stay; accept loss of ±25° sweep QA.
- **[Risk] External bookmark/scripts call `applyPose`** → Mitigation: none expected (docs site debug hook); document removal in proposal as BREAKING viewer API.
- **[Trade-off] Cannot demo firmware pose scale in the viewer** → Accepted; robot + API remain the source of truth for motion.

## Migration Plan

1. Land code + docs + tests in one PR.
2. Next Pages deploy serves unchanged GLB path; chrome simply lacks joint controls.
3. Rollback: revert the PR (restore modules, `check_poses` in `build.sh`, UI).

## Open Questions

None — fork B and out-of-scope CAD checks already decided.
