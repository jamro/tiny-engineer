# Tasks

## 1. Settings core

- [x] 1.1 Add `FacePose` (`set`, `values[3]`), `SettingsValues` fields with not-set default, `SettingsValues::log()` output, and `settingsFacePose()` getter; verify `pio run` compiles
- [x] 1.2 Add `FacePose::valid()` (not set, or each value finite and −1..1; mirrors `ServoRanges::valid()`) and wire it into `SettingsPatch::invalidField()` and `SettingsValues::apply()`; do not add it to `setupOnlyStep()`; verify new cases in `test/test_settings_validate` (valid set, valid clear, out of range, NaN) pass with `pio test -e native`
- [x] 1.3 Add NVS key `kKeyFacePose = "face_pose"` as a 12-byte float blob: load (missing or wrong size = not set), write on set, remove key on clear; verify factory reset leaves it cleared by reading `factoryResetSettings()` and that `pio run` compiles
- [x] 1.4 Document the field in `docs/settings.md` (HTTP field list, NVS key list, factory-reset pitfall) and verify the lists match `nvs.h` and `readPatch()`

## 2. HTTP API

- [x] 2.1 Add `face_pose` to `settingsJson()` (`[h,n,b]` or `[]`) and to `readPatch()` with a `readField` overload for the 0-or-3 float array (null = wrong type); grow `known[]`; verify `pio run` compiles
- [x] 2.2 Update `docs/api.md` (GET example, field table, POST param table, `[]` clears, apply timing, factory reset) and the `/settings` param list in `ui/src/views/ApiReference.tsx` with identical values; verify both texts match
- [x] 2.3 Add `face_pose` to the `docs/hardware/testing.md` settings rows (`README.md` lists no settings fields, so it needs no change); verify with `grep -n face_pose docs/hardware/testing.md`

## 3. Talking animation

- [x] 3.1 Add `anim::FacePose` and `anim::facePose()` (fallback `{TALKING_HEAD_MID, 0, 0}`) in `src/animation/util.*`; limit targets with the existing `servoSaturateNorm()`; verify `pio run` compiles
- [x] 3.2 Update `src/animation/talking.cpp` per design D5: read the face pose once in `startTalking()`, rest head/neck/body on it, neck drift and body sway around it with clamp, body bias relative to face neck, settle body to face body; verify by code review that every former `TALKING_HEAD_MID` / `0.0f` neck and body target now uses the cached pose and that the not-set pose gives the same numbers as before; `pio run` passes
- [x] 3.3 Add a "Face me" paragraph to `docs/robot-movement.md` (what it is, units, only talking uses it, recalibration note); verify the doc links to `docs/api.md`

## 4. Web UI: Servo control section

- [x] 4.1 Add `face_pose: number[]` to `Settings` in `ui/src/api/types.ts` and a `normToDeg(n, range)` helper in `ui/src/servos.ts`; verify `npm run lint --prefix ui`
- [x] 4.2 Add a shared `FacePoseControls` component in `ui/src/components/` (three −1..1 range inputs for head, neck, body; calls an injected move function with degrees) and a `FacePoseSection` on `ServoControl.tsx` (status Set / Not set, Save, Clear with confirm, Preview via `POST /anim` `name=talking`, recalibration note); verify with new cases in `ui/test/servo.test.tsx` (start values, move request angle, save body, clear body) and `npm test --prefix ui`

## 5. Web UI: setup wizard step

- [x] 5.1 Add step `{ id: "face", title: "Face me" }` after `servos` in `ui/src/setup/Wizard.tsx` with a `FaceStep` that uses `FacePoseControls` with `useSetupServo`, Skip (sends nothing) and Save and continue (posts `face_pose`); verify with new cases in `ui/test/wizard.test.tsx` (step order, Skip sends no `face_pose`, Save sends it) and `npm test --prefix ui`, plus `npm run lint --prefix ui`

## 6. Integration

- [x] 6.1 Run `pio run`, `pio test -e native`, `npm run lint --prefix ui` and `npm test --prefix ui`; verify all pass
- [x] 6.2 Run `openspec validate face-me --strict`; verify it passes
- [x] 6.3 On the real robot: with no face pose, talking looks as before; set a face pose from `/servo`, preview talking, and confirm the robot rests and sways around the user; clear it and confirm talking returns to the old pose; run the wizard Face me step with Skip and with Save (user test)

## 7. Feedback from the first robot test

- [x] 7.1 Add top margin to the Face me section on `/servo`; verify by `npm run lint --prefix ui` and a visual check on the robot
- [x] 7.2 Replace the face sliders with a joint picker, a min..max range bar with a marker, whole-degree angles and ±1° / ±5° nudge buttons (shared `FacePoseControls`, used on `/servo` and in the wizard); verify updated cases in `ui/test/servo.test.tsx` and `ui/test/wizard.test.tsx` pass with `npm test --prefix ui`
- [x] 7.3 Make the first reading neck turn use the typing sway speed (shared `TYPING_SWAY_SPEED_*` constants); verify `pio run -e supermini` passes and, on the robot, the transition into reading feels like typing
- [x] 7.4 Make talking turn neck and body to the face pose at 40°/s and the head with the typing head snap, and hold neck drift, body sway, reactive head settle and emphasis nod until all three arrive (also on resume after a cancelled transition); verify `pio run -e supermini` passes
- [x] 7.5 On the real robot: confirm 7.1–7.4 (user test)

## 8. Review feedback (PR #116, CodeRabbit)

- [x] 8.1 Move all three face joints to the shown pose before any value can be saved: on open in the wizard step, before the first nudge on `/servo` (shared `FacePoseControls`); verify new cases in `ui/test/servo.test.tsx` and `ui/test/wizard.test.tsx`
- [x] 8.2 Send no manual move when stopping the talking preview fails; verify a new case in `ui/test/servo.test.tsx`
- [x] 8.3 Reset the controls to the default pose after a successful Clear; verify a new case in `ui/test/servo.test.tsx`, plus `npm run lint --prefix ui` and `npm test --prefix ui`
- [x] 8.4 On the real robot: wizard step opens at the shown pose, first nudge on `/servo` syncs all joints, Clear resets the controls (user test)
- [x] 8.5 Second review: Save on `/servo` and Save and continue in the wizard move all three joints to the shown pose first and save only when all moves succeed; a move from the Servo control form makes the next nudge sync again; verify new cases (save without a nudge, failed move before save, resync after a form move) in `ui/test/servo.test.tsx` and `ui/test/wizard.test.tsx`, plus `npm run lint --prefix ui` and `npm test --prefix ui`
- [x] 8.6 On the real robot: Save without a nudge moves the robot to the shown pose; a form move then a nudge syncs all joints (user test)
- [x] 8.7 Third review: a preview start or stop makes the next nudge sync all joints again; verify a new case in `ui/test/servo.test.tsx`, plus `npm run lint --prefix ui` and `npm test --prefix ui`
- [x] 8.8 On the real robot: preview, stop, then nudge syncs all joints (user test)

## 9. Fold Face me into guided calibration

- [x] 9.1 Remove the outer Face me wizard step; open Face me as the last guided calibration substep (`11 of 11`); hub Skip skips Face me; shorten copy and match guided chrome; verify `ui/test/wizard.test.tsx`, `npm test --prefix ui`, and `npm run lint --prefix ui`
- [x] 9.2 Update `docs/api.md`, `docs/robot-movement.md`, and the face-me OpenSpec artifacts for the five-step outer wizard and Face-me-inside-calib behavior

## 10. Keep face pose on factory reset

- [x] 10.1 Preserve `facePose` in `factoryResetSettings()` like servo ranges; update UI/CLI/docs/OpenSpec; verify `pio run`

## Workflow follow-up

- After the robot test passes, rebase the branch onto `main` in a separate worktree (`git rebase --onto main custom_setup feat/face-me`), run `pio run`, push to `fork` and open the PR to `jamro/tiny-engineer` after user approval.
- Archive the change after the PR is merged.
