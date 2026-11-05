# Tasks

## 1. Pose cleanup

- [x] 1.1 Remove `blendPoses` from `web/js/assembly/poses.js` and drop its imports/asserts in `scripts/assembly/test_viewer.mjs` — verify `node scripts/assembly/test_viewer.mjs` passes and `rg blendPoses web scripts` is empty
- [x] 1.2 Confirm `REST` / `CENTRE` / `normalizePose` still used by viewer and `check_poses.mjs` — verify those symbols remain exported and imported

## 2. Colour path cleanup

- [x] 2.1 Slim `web/js/assembly/colors.js`: remove `normalizeHex` and the `part.color` branch in `defaultColor`; drop unused `FASTENER_FINISH.name`; keep tables + `defaultColor` / `resolveColors` — verify module has no `normalizeHex` export
- [x] 2.2 Stop forwarding `entry.color` / `entry.mod` in `scripts/assembly/build_glb.mjs` and stop reading `extras.color` in `web/js/assembly/main.js` — verify `rg 'extras\.color|entry\.color|entry\.mod' web/js/assembly scripts/assembly` is empty (aside from unrelated STL vertex colour code)
- [x] 2.3 Update `scripts/assembly/test_viewer.mjs` colour tests for table-only resolution (no hex XSS cases); keep group/part/hardware coverage — verify `node scripts/assembly/test_viewer.mjs` and `node scripts/assembly/test_build_glb.mjs` pass

## 3. Sanity

- [x] 3.1 Grep for leftover docs/comments that describe pose blending or per-part colour overrides in assembly viewer files; fix only if inaccurate — verify no stale claim of picker/share hex or `blendPoses` in `docs/3d/assembly-viewer.md` or assembly JS headers
- [x] 3.2 Run `node scripts/assembly/test_viewer.mjs` and `node scripts/assembly/test_build_glb.mjs` once more after all edits — verify both exit 0

## Workflow follow-up

- Archive with `/opsx-archive` after implementation (no spec sync; `skip_specs: true`).
