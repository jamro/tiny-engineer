# Tasks

## 1. Calibration state and hub

- [ ] 1.1 Refactor `useCalibration` (or split a guided hook) to modes `hub | assembly | guided`, substep index for guided, and ranges/angles; verify TypeScript builds via `npm run lint --prefix ui`
- [ ] 1.2 Add a servos hub UI wired from `Wizard` as the default servos entry (Calibrate / Skip / Assembly); verify a Vitest case that provisioning opens the hub (no auto horns/ranges)
- [ ] 1.3 Implement Skip: save current `servo_mins`/`servo_maxs` via existing settings path and advance to Screen; verify wizard test asserts POST payload matches loaded defaults/settings and step becomes Screen

## 2. Assembly path (Stage A)

- [ ] 2.1 Adapt assembly UI: move-all-to-90° plus per-servo Identify (±3° around 90° via `/setup/servo`); Done/Back returns to hub only; verify Vitest covers all-90 POST and an identify sequence for one index, and that finishing assembly shows the hub again

## 3. Guided path (Stage B)

- [ ] 3.1 Implement Body → Neck → Head substeps: center at 90°, set min with ±1/5/10 nudges, mirror max (`180 − min`), reject min ≥ 90, demo max→center with setup moves; verify Vitest for body order, mirrored max in ranges state, and demo `/setup/servo` calls
- [ ] 3.2 Implement Hands substeps in docs polarity order (R max, L min, L max, R min) with landmark copy and no centers; verify Vitest that confirming those steps writes the four bounds correctly
- [ ] 3.3 Wire guided Back (previous substep / hub) and final confirm → save ranges → Screen; verify Vitest for Back-to-hub and save-after-complete payload
- [ ] 3.4 Remove or stop using free joint-tab `FindRanges` during guided mode; verify no joint-tab switching appears in guided tests / UI

## 4. Docs touch-up (if copy still describes old flow)

- [ ] 4.1 Update any human docs that describe forced “parts on → joint tabs” setup calibration to mention hub / guided / skip / assembly; verify grep for obsolete wizard phrases is clean or intentional

## Workflow follow-up

- Archive the change after implementation and review (`/opsx-archive` or project equivalent).
- Sync specs into `openspec/specs/` on archive if that is the project’s archive path.
