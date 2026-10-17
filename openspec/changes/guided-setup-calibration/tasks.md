# Tasks

## 1. Calibration state and hub

- [x] 1.1 Refactor `useCalibration` (or split a guided hook) to modes `hub | assembly | guided`, substep index for guided, and ranges/angles; verify TypeScript builds via `npm run lint --prefix ui`
- [x] 1.2 Add a servos hub UI wired from `Wizard` as the default servos entry (Calibrate / Skip / Assembly); verify a Vitest case that provisioning opens the hub (no auto horns/ranges)
- [x] 1.3 Implement Skip: save current `servo_mins`/`servo_maxs` via existing settings path and advance to Screen; verify wizard test asserts POST payload matches loaded defaults/settings and step becomes Screen

## 2. Assembly path (Stage A)

- [x] 2.1 Adapt assembly UI: move-all-to-90° plus per-servo Identify (±3° around 90° via `/setup/servo`); Done/Back returns to hub only; verify Vitest covers all-90 POST and an identify sequence for one index, and that finishing assembly shows the hub again

## 3. Guided path (Stage B)

- [x] 3.1 Implement Body → Neck → Head substeps: center at 90°, set min with ±1/5/10 nudges, mirror max (`180 − min`), reject min ≥ 90, demo max→center with setup moves; verify Vitest for body order, mirrored max in ranges state, and demo `/setup/servo` calls
- [x] 3.2 Implement Hands substeps in docs polarity order (R max, L min, L max, R min) with landmark copy and no centers; verify Vitest that confirming those steps writes the four bounds correctly
- [x] 3.3 Wire guided Back (previous substep / hub) and final confirm → save ranges → Screen; verify Vitest for Back-to-hub and save-after-complete payload
- [x] 3.4 Remove or stop using free joint-tab `FindRanges` during guided mode; verify no joint-tab switching appears in guided tests / UI
- [x] 3.5 Guided center steps: same ±1/5/10 nudges as min/max; record session-only center in client state (not NVS); mirror max as `2×center − min`; reject `min >= center`; demo returns to session center; update Vitest (including save payload still mins/maxs only)
- [x] 3.6 Default guided center park = midpoint of current min/max per joint; update Vitest
- [x] 3.7 Swap guided hand bound bindings (R min / L max at keyboard; L min / R max at upper) and landmark copy; update Vitest
- [x] 3.8 Park guided min / hand-min at current `ranges[joint].min` and hand-max at current `ranges[joint].max` (parallel to center → mid); drop offset parks (`center−20`, electrical mid ±20); update Vitest

## 4. Docs touch-up (if copy still describes old flow)

- [x] 4.1 Update any human docs that describe forced “parts on → joint tabs” setup calibration to mention hub / guided / skip / assembly; verify grep for obsolete wizard phrases is clean or intentional
- [x] 4.2 Align `docs/robot-movement.md` (and any hand at-min/at-max copy) with the bench-proven guided hand polarity; verify docs and UI bindings match

## 5. Stock empty-NVS defaults

- [x] 5.1 Set `SERVO_SPECS` (and settings default fill) to min 80 / max 100 for all five joints; sync UI `DEFAULT_SERVO_RANGES`, docs tables, and tests; verify `pio test -e native` and UI skip-save payload use `[80×5]` / `[100×5]`

## 6. Hub and guided UX polish

- [x] 6.1 Hub: label **Calibrate servos** (primary), **Assembly support** (secondary mid-weight), **Skip** last/quiet; add when-to-use lines; update Vitest button names/order
- [x] 6.2 Emphasize landmarks/stop cues with `<strong>` (or equivalent) in hub, assembly, and guided instruction copy; update Vitest where copy is asserted
- [x] 6.3 Assembly: per-joint identify done feedback; clearer Done CTA; update Vitest
- [x] 6.4 Guided: progress shows joint context; simplify confirm labels (no “demo” in button text); update Vitest
- [x] 6.5 Soft skip hint for stock 80–100 ranges; update Vitest (payload still mins/maxs only)

## 7. Wizard usability follow-up

- [x] 7.1 Simplify the servo hub to one short intro plus self-describing action buttons (title + hint); remove the duplicated when-to-use list; update Vitest
- [x] 7.2 Show guided role once in the heading, add `Servo calibration · N of 10` plus an independent internal progress bar; update Vitest
- [x] 7.3 Label the calibration track with current saved min/max and add concise per-substep target hints; update Vitest
- [x] 7.4 Make Screen rotation secondary and display current Normal / Rotated 180° preview state; update Vitest
- [x] 7.5 Rename the final guided CTA to **Save and continue**; keep Back neutral and visibly focusable across wizard screens; update Vitest
- [x] 7.6 Show persistent successful completion / selected state for RGB actions and Speaker playback; update Vitest
- [x] 7.7 Preserve a session draft angle per guided substep; Back restores and moves to the draft; leaving/restarting guided clears drafts; update Vitest
- [x] 7.8 Add a short-viewport sticky wizard action footer with safe-area spacing and no content overlap; verify responsive CSS and UI lint
- [x] 7.9 Before state mutation or demo, reject Body/Neck/Head mirrored `nextMax > 180`; show an error, remain on min, keep ranges unchanged, and send no out-of-range move; add Vitest regression
- [x] 7.10 Remove stale final ranges-summary expectations from planning/tests; verify `npm run lint --prefix ui` and `npm test --prefix ui`
- [x] 7.11 Gate Identify until successful Move all to 90° in the current assembly visit (session `centered` flag; clear on leave); keep Identify disabled on failure; Vitest for disabled → enabled after center-all and identify sequence

## Workflow follow-up

- Archive the change after implementation and review (`/opsx-archive` or project equivalent).
- Sync specs into `openspec/specs/` on archive if that is the project’s archive path.
