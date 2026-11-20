# Design

## Context

- Animations author poses in −1..1 relative to each joint's saved min/max and map them with `servoNormToDeg` (`src/hardware/servo_wrapper.cpp`). Saved ranges (`sranges`) are setup-AP-only.
- Talking (`src/animation/talking.cpp`) rests at `TALKING_HEAD_MID` (1/7), neck 0 and body 0. Neck drift is `±TALKING_NECK_SWAY` around 0. Body sway is `±TALKING_BODY_SWAY` around 0, biased opposite the neck. `startTalking()` calls `parkNonePose()`, which moves neck and body to 0.
- Settings follow the layer checklist in `docs/settings.md`: `SettingsValues`, `SettingsPatch` with `std::optional` fields, NVS keys in `nvs.h`, `readPatch()` / `settingsJson()` in `src/http/settings_handlers.cpp`. `SettingsPatch::setupOnlyStep()` lists setup-AP-only fields.
- `/test/servo` (Configured) moves one joint in degrees inside the saved range. `/setup/servo` (Setup) does slow 0–180° moves in setup AP mode. `/anim` is Configured, so it is not available in setup AP mode.
- Each web UI page needs its own `handleIndex` entry in `kRoutes` (`src/http/routes.cpp`).

## Goals / Non-Goals

**Goals:**
- Zero behavior change when `face_pose` is not set.
- One small helper that animations call to get the face pose, so later animations can adopt it with a one-line change.

**Non-Goals:**
- Welcome, attention and other animations (later changes).
- Serial `settings.get` / `settings.set` and CLI support for `face_pose`.
- Changing the transition park: between animations, neck and body still park at 0.
- Auto-adjusting the pose after a servo range recalibration.

## Decisions

### D1. Storage: three floats plus a "set" flag, one NVS blob
`SettingsValues` gets `bool facePoseSet` and `float facePose[3]` (head, neck, body). NVS key `face_pose` (9 chars) holds a 12-byte blob of three floats; no key means not set. Clearing removes the key.
- *Alternative:* three `int8` values in hundredths. Smaller, but adds a scale factor in API, UI and firmware for no real gain.
- *Alternative:* NaN as "not set". Fragile with JSON and comparisons.

### D2. API shape: array, `[]` clears
`GET` returns `[h, n, b]` or `[]`. `POST` takes three numbers or `[]`. `null` stays a wrong type, matching the existing `readPatch()` rule "an explicit null is a wrong type". The patch type is `std::optional<FacePose>`, where `FacePose` has `set` and `values[3]`. Validation lives in `FacePose::valid()` (same pattern as `ServoRanges::valid()`) and runs in `SettingsPatch::invalidField()`, so the whole patch is rejected before any NVS write. `face_pose` is not added to `setupOnlyStep()`. The `known[]` array in `readPatch()` grows by one.
- *Alternative:* separate `face_pose_set` boolean. Two fields that must agree; more checks.

### D2b. Factory reset keeps face pose
`factoryResetSettings()` copies `facePose` into the reset snapshot alongside servo ranges, RGB order, and OLED rotation. Explicit Clear on `/servo` or `POST {"face_pose":[]}` still clears it.

### D3. Normalized units (user choice)
The pose uses the same −1..1 units as animations, so talking adds offsets with no conversion. The trade-off (direction shift after recalibration) is handled in the UI text (see Risks).

### D4. Helper: `anim::facePose()`
In `src/animation/util.*`: `struct FacePose { float head, neck, body; }` and `FacePose facePose()`. It returns the saved pose, or `{TALKING_HEAD_MID, 0, 0}` when not set. The fallback is the current talking rest pose, which gives the goal "zero change". Targets with offsets go through the existing `servoSaturateNorm()` (`include/servos.h`), which limits them to −1..1; no new clamp helper.

### D5. Talking changes
- `startTalking()`: after `parkNonePose()`, set head, neck and body targets to the face pose (when not set, the targets equal today's: head 1/7, neck 0, body 0).
- `TALKING_HEAD_MID` uses → `facePose().head`. Nod and level offsets stay the same and are clamped.
- `commandNeckDrift()`: `face.neck + TALKING_NECK_SWAY * u`, clamped.
- `commandBodyDrift()`: `face.body + TALKING_BODY_SWAY * u`, clamped. The opposite-neck bias compares the neck to `face.neck`, not to 0.
- `settleReactivePose()`: body returns to `face.body`.
Read `facePose()` once per `startTalking()` and keep it in a file-local variable, so a mid-animation save does not jump the pose. A new save applies the next time talking starts (spec: "without reboot").

### D6. Web UI: section on the Servo control page
A `FacePoseSection` component on `ServoControl.tsx` (`/servo`), so no new route in `kRoutes`. A shared `FacePoseControls` component: a joint picker (Head / Neck / Body, each showing its angle), a large angle readout in whole degrees, a bar that spans the joint's saved min..max with a marker at the current angle, and ±1° / ±5° nudge buttons (the guided calibration buttons without ±10°: the face range is narrow). A nudge adds degrees to the rounded current angle, limits it to the saved range, moves the joint at once, and stores the result back in −1..1 units (`normToDeg` / `degToNorm` in `ui/src/servos.ts`, the same mapping as the firmware). A failed move restores the previous value. The section has extra top margin so it does not touch the move form above it. Save (on `/servo` and in the wizard) first moves all three joints to the shown pose and saves only when every move succeeds, so a saved angle is always one the robot reached, with or without a nudge. Before the first nudge the joints also sync (the wizard step does it on open); a move from the Servo control form on the same page, or a preview start or stop (stop parks neck and body), makes the next nudge sync again. `/servo` does not move on page load. A move is not sent while a preview runs and cannot be stopped. After Clear the controls go back to the default pose (review feedback on PR #116).
- *Replaced:* −1..1 sliders (first version), then ±10° buttons and decimal angles with no range bar (second version). The user found both not precise or clear enough on the robot. Buttons: Save, Clear (with confirm), Preview (`POST /anim` `name=talking`). The section shows "Set" or "Not set" and the recalibration note.

### D7. Face me inside guided calibration
Face me is not an outer wizard step. After the last guided range confirm, the UI posts servo mins/maxs and opens Face me as calibration mode `face` (progress includes it, e.g. `11 of 11`). The step reuses `FacePoseControls` with `useSetupServo`, matches guided chrome (progress, target, in-section footer), and offers Back (to the last range substep) plus "Save and continue" (move then POST `face_pose`, then advance the outer wizard). No Skip on Face me; hub Skip skips Face me entirely. No preview in setup AP mode (`/anim` unavailable). Outer wizard stays five steps: Servos → Screen → RGB → Speaker → Connection.

## Risks / Trade-offs

- [Servo range recalibration shifts the physical direction] → UI note on the Face me section and on the wizard step; the face pose stays valid (still inside −1..1), only the direction can change.
- [Face neck near ±1 makes drift hit the limit and look stuck on one side] → clamp keeps it safe; acceptable for v1, users can pick a pose away from the limit.
- [Moving joints from the UI while an animation runs] → `/test/servo` already handles that case for the Servo control page; same behavior.
- [Transition into reading turned the neck too slowly] A transition does not wait for the neck to park, so after talking with a face pose the neck can start far from mid. Reading turned it at its slow 8–16°/s scan speed. → The first reading neck turn now uses the typing sway speed (12–30°/s, shared constants in `constants.h`); later turns keep the scan speed. Found during the robot test.
- [Turn to the face pose was too slow when talking started] `startTalking()` aimed neck and body at the face pose, but the first neck drift (after 0.5–1.1 s, 8–16°/s) and body sway or reactive settle (10–14°/s) replaced that target, so most of a long turn ran at drift speed. → Neck and body turn at `TALKING_FACE_ARRIVE_SPEED_DEG_S` (40°/s), and neck drift, body sway and the reactive body settle wait until both arrive. The same applies when a cancelled transition resumes talking. Second robot test: still slower than typing, because the head moved to the face pose at 25°/s and the reactive settle held it at 22°/s, while typing snaps the head at 0.85 × max speed. → The head now uses that same snap (`snapHeadToRangeHigh`), the arrival waits for head, neck and body, and the reactive head settle and emphasis nod also wait. Without a face pose this changes only the speed of the short turn to the rest pose, not any pose.
- [Upstream review: new field on a shared API] → additive field, empty by default; docs synced in the same change per CLAUDE.md.

## Migration Plan

No migration. Older firmware ignores the missing key; new firmware treats a missing key as not set. Rollback: flash the older firmware; the `face_pose` NVS key stays unused.
