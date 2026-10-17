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

## 4. Docs touch-up (if copy still describes old flow)

- [x] 4.1 Update any human docs that describe forced “parts on → joint tabs” setup calibration to mention hub / guided / skip / assembly; verify grep for obsolete wizard phrases is clean or intentional
- [x] 4.2 Align `docs/robot-movement.md` (and any hand at-min/at-max copy) with the bench-proven guided hand polarity; verify docs and UI bindings match

## Workflow follow-up

- Archive the change after implementation and review (`/opsx-archive` or project equivalent).
- Sync specs into `openspec/specs/` on archive if that is the project’s archive path.
