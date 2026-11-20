# Proposal

## Why

Users put the robot on their desk in different positions relative to where they sit. The talking animation "faces the user" with a fixed guess (neck mid, body mid, head slightly up), so on most desks the robot talks to an empty spot. A per-user "face me" pose lets each user point the robot at themselves once, and animations that address the user can use it.

## What Changes

- New optional setting `face_pose`: a head (pitch), neck (yaw) and body (yaw) pose in the normalized −1..1 animation units. Empty (`[]`) means not set. Not set is the default.
- When `face_pose` is not set, the robot moves exactly as today. No user has to do this setup.
- The talking animation centers its head nods, neck drift and body sway on the face pose when it is set. Other animations do not change in this change.
- `GET /settings` returns `face_pose`. `POST /settings` accepts `face_pose` (three numbers to set, `[]` to clear) in normal mode and in setup AP mode. Factory reset clears it.
- Web UI: a new **Face me** section on the Servo control page (`/servo`) to move head, neck and body to the user, save, clear and preview the talking animation. No new page route.
- Setup AP wizard: an optional **Face me** step after the servos step, with Skip.
- Docs: `docs/api.md`, `docs/settings.md`, the web UI API reference, `README.md`, `docs/robot-movement.md`.

## Capabilities

### New Capabilities
- `face-pose`: the per-user "face me" pose — storage, settings API, default behavior when not set, calibration in the web UI and the setup wizard, and which animations use it.

### Modified Capabilities
- `setup-servo-calibration`: the "Guided calibration is strict forward with Back" requirement names the "outer five-step wizard progress". The wizard now has six steps, so the text says "outer wizard progress" (no count). The servos step itself does not change; the new Face me step is a `face-pose` requirement.

## Impact

- Firmware: `src/settings/` (new field, NVS key, validation, getter), `src/http/settings_handlers.cpp`, `src/animation/talking.cpp`, `src/animation/util.*` (face pose helper).
- Web UI: `ui/src/api/types.ts`, new Face me section on `ui/src/views/ServoControl.tsx`, `ui/src/setup/Wizard.tsx` and a new wizard step, `ui/src/views/ApiReference.tsx`.
- Tests: `test/test_settings_validate` (native), UI unit tests.
- API: additive field on `/settings`. No breaking change. Clients that do not know `face_pose` keep working.
- Known trade-off: the pose is stored relative to the saved servo min/max. After a servo range recalibration the physical direction can shift, so the UI tells the user to check Face me again.
