# Spec Delta

## Purpose

Defines the setup AP wizard servo step: assembly centering and identify, guided range calibration for an assembled robot, and skip — without changing how ranges are stored or which HTTP endpoints move servos.

## ADDED Requirements

### Requirement: Servos step presents a hub
While the setup wizard is on the servos step and neither assembly nor guided calibration is active, the UI MUST present a hub with three actions in this order: **Calibrate servos** (primary), **Assembly support** (secondary / mid visual weight), and **Skip** (least visual emphasis). Each action MUST include short “when to use” guidance without repeating the same choices in a separate list.

#### Scenario: Hub is the default entry
- **WHEN** the user opens the setup wizard on the servos step
- **THEN** the hub is shown before assembly or guided calibration content

#### Scenario: Hub action labels and order
- **WHEN** the hub is shown
- **THEN** the primary action is labeled Calibrate servos
- **AND** the secondary action is labeled Assembly support
- **AND** Skip appears after Assembly support with quieter styling than Assembly support

#### Scenario: Skip advances without guided calibration
- **WHEN** the user chooses Skip from the hub
- **THEN** the wizard advances past the servos step without entering guided range calibration
- **AND** the device retains the servo min/max values already loaded for that step (NVS or defaults)

#### Scenario: Soft hint when skipping stock defaults
- **WHEN** the user is on the hub and current ranges are the stock 80° / 100° defaults
- **THEN** the UI shows a soft hint that skipping keeps the narrow stock band
- **AND** the hint does not block Skip

### Requirement: Assembly path centers and identifies then returns
The assembly path MUST let the user move all servos to electrical 90° and identify each servo by a small oscillation around 90°, then MUST return only to the hub (not into guided calibration).

#### Scenario: Center all at midpoint
- **WHEN** the user requests move-all-to-midpoint on the assembly path
- **THEN** every joint moves to 90° via the existing setup servo move endpoint

#### Scenario: Identify disabled before center-all
- **WHEN** the assembly path is open and move-all-to-midpoint has not succeeded in this visit
- **THEN** Identify controls for every joint are disabled

#### Scenario: Identify enabled after successful center-all
- **WHEN** move-all-to-midpoint succeeds on the assembly path
- **THEN** Identify controls become enabled

#### Scenario: Identify one servo
- **WHEN** the user identifies a joint on the assembly path after successful center-all
- **THEN** that joint moves a few degrees below and above 90° (about ±3°) and returns to 90°
- **AND** other joints are not commanded by that identify action
- **AND** that joint’s identify control shows a done state until the user leaves assembly

#### Scenario: Leave assembly to hub only
- **WHEN** the user finishes or leaves the assembly path
- **THEN** the UI returns to the servos hub
- **AND** guided range calibration does not start automatically
- **AND** the next assembly visit starts with Identify disabled again until center-all succeeds

### Requirement: Guided calibration is strict forward with Back
Guided range calibration MUST proceed in a fixed joint order with only forward confirmation and Back to the previous substep (or to the hub from the first substep). Free joint-tab switching MUST NOT be offered during this path.

#### Scenario: Joint order
- **WHEN** the user completes guided calibration from the start
- **THEN** the path visits Body, then Neck, then Head, then Hands, in that order

#### Scenario: Back from first substep
- **WHEN** the user is on the first guided substep and chooses Back
- **THEN** the UI returns to the servos hub

#### Scenario: Back within the path
- **WHEN** the user is past the first guided substep and chooses Back
- **THEN** the UI returns to the previous guided substep

#### Scenario: Progress shows joint context
- **WHEN** the user is on a guided substep
- **THEN** the section heading shows the joint and center/min/max or hand landmark role once
- **AND** a separate line shows `Servo calibration` and the current substep out of all guided substeps
- **AND** an internal progress indicator advances independently of the outer five-step wizard progress

### Requirement: Instruction copy emphasizes key phrases
Hub, assembly, and guided instruction text MUST emphasize landmarks and stop conditions with strong/bold phrasing so the eye can scan the important cues.

#### Scenario: Guided landmark emphasis
- **WHEN** the user is on a guided substep with landmark instructions
- **THEN** the landmark and stop-condition phrases are visually stronger than surrounding copy

### Requirement: Body neck and head use center then mirrored range
For Body, Neck, and Head, guided calibration MUST set a session center, then a min below it, then a max mirrored about that center, then demo the range ending at the session center before continuing.

#### Scenario: Center uses nudge controls
- **WHEN** the user is on a Body, Neck, or Head center substep
- **THEN** the UI offers the same ±1°, ±5°, and ±10° nudge buttons used for min/max steps
- **AND** the joint starts at the midpoint of its current min and max
- **AND** confirming the step records the current angle as that joint’s session center

#### Scenario: Calibration controls explain range and target
- **WHEN** the user is on any guided substep
- **THEN** the track identifies the current saved minimum and maximum
- **AND** concise target text states the physical landmark the user is adjusting toward

#### Scenario: Min step opens at current min
- **WHEN** the user enters a Body, Neck, or Head min substep
- **THEN** the joint starts at that joint’s current minimum from the ranges held for the step

#### Scenario: Body landmarks
- **WHEN** the user sets Body min after a recorded center
- **THEN** copy instructs rotating the body toward the robot’s right until the right hand is over the bell
- **AND** max is set to `2 × center − min`

#### Scenario: Neck min is user-chosen
- **WHEN** the user sets Neck min after a recorded center
- **THEN** copy does not require a bell landmark
- **AND** max is set to `2 × center − min`

#### Scenario: Head landmarks
- **WHEN** the user sets Head center and min
- **THEN** center copy describes looking straight forward
- **AND** min copy describes looking at the computer screen
- **AND** max is set to `2 × center − min`

#### Scenario: Range demonstration after mirrored max
- **WHEN** a mirrored max has just been established for Body, Neck, or Head
- **THEN** the joint moves slowly to that max, then slowly back to the recorded session center, before the next substep

#### Scenario: Mirrored max exceeds electrical range
- **WHEN** a Body, Neck, or Head min would produce `2 × center − min > 180`
- **THEN** the UI shows an error and remains on the current min substep
- **AND** the in-memory range is not changed
- **AND** no out-of-range setup servo move or demo is requested

#### Scenario: Center is not persisted
- **WHEN** the user finishes guided calibration and advances
- **THEN** the settings update includes `servo_mins` and `servo_maxs` only
- **AND** no center angle is written to NVS or settings

#### Scenario: Confirm labels stay simple
- **WHEN** the user is on a center, min, or hand bound substep
- **THEN** the primary confirm control uses short wording (Confirm center / Confirm min / Confirm max or equivalent landmark wording)
- **AND** the label does not require the word “demo”

#### Scenario: Final guided action describes navigation
- **WHEN** the user reaches the final hand-bound substep
- **THEN** the primary action is labeled Save and continue
- **AND** no ranges-summary box is shown

### Requirement: Hands calibrate four bounds without centers
Guided Hands calibration MUST set four electrical bounds in this order, with no center step: right-hand min (keyboard height with slack), left-hand max (same height), left-hand min (upper), right-hand max (same upper height).

#### Scenario: Hand bound step opens at current bound
- **WHEN** the user enters a hand bound substep for min or max
- **THEN** the joint starts at that joint’s current minimum or maximum (respectively) from the ranges held for the step

#### Scenario: Keyboard height bounds
- **WHEN** the user sets the keyboard-height pose for both hands
- **THEN** the right-hand electrical min and left-hand electrical max are recorded

#### Scenario: Upper height bounds
- **WHEN** the user sets the upper pose for both hands
- **THEN** the left-hand electrical min and right-hand electrical max are recorded

### Requirement: Calibration moves are slow setup moves
All servo motion during assembly and guided calibration MUST use the existing setup-AP slow move endpoint (`POST /setup/servo`), not an instantaneous position snap and not the post-provisioning test servo endpoint.

#### Scenario: Nudge during guided calibration
- **WHEN** the user nudges a joint during guided calibration
- **THEN** the UI requests a setup servo move for that joint and angle
- **AND** available nudge steps include ±1°, ±5°, and ±10°

#### Scenario: Back restores substep draft
- **WHEN** the user revisits a guided substep with Back after nudging it
- **THEN** the displayed angle and setup servo move use that substep’s session draft
- **AND** the draft is discarded when guided calibration is left or restarted

### Requirement: Completing or skipping servos persists ranges unchanged in mechanism
Leaving the servos step after guided calibration or skip MUST persist `servo_mins` and `servo_maxs` through the existing settings update path. Skip MUST NOT invent new range values beyond what the step already held.

#### Scenario: Save after guided calibration
- **WHEN** the user finishes guided calibration and advances to the next wizard step
- **THEN** the client posts the calibrated `servo_mins` and `servo_maxs` via `POST /settings`

#### Scenario: Save after skip
- **WHEN** the user skips calibration and advances
- **THEN** the client posts the ranges already held for the step (from settings/defaults), without requiring the user to set new limits

### Requirement: Empty NVS uses narrow stock ranges
When no servo ranges are stored in NVS, the device MUST use stock defaults of minimum 80° and maximum 100° for every joint (head, neck, left hand, right hand, body).

#### Scenario: Settings expose stock defaults
- **WHEN** settings are loaded with empty servo-range NVS
- **THEN** `servo_mins` is `[80, 80, 80, 80, 80]`
- **AND** `servo_maxs` is `[100, 100, 100, 100, 100]`

#### Scenario: Skip without prior calibration saves stock defaults
- **WHEN** the user skips calibration on a device still on stock defaults
- **THEN** the posted `servo_mins` and `servo_maxs` match those 80° / 100° arrays

### Requirement: Remaining wizard actions communicate hierarchy and completion
Screen rotation, RGB testing/mapping, Speaker playback, and wizard navigation MUST communicate which action advances the wizard and whether device feedback has completed.

#### Scenario: Screen rotation is secondary
- **WHEN** the user is on the Screen step
- **THEN** Rotate 180° has less visual emphasis than the Next action
- **AND** the UI displays whether the current preview is Normal or Rotated 180°

#### Scenario: RGB and Speaker show completion
- **WHEN** an RGB test/mapping action or Speaker playback succeeds
- **THEN** the relevant step displays a persistent completion or selected state

#### Scenario: Back remains consistent and accessible
- **WHEN** Back is shown on any wizard or guided screen
- **THEN** it uses the same neutral visual hierarchy
- **AND** keyboard focus remains visibly indicated

#### Scenario: Actions remain reachable on short viewports
- **WHEN** wizard content is taller than a short mobile viewport
- **THEN** the action footer remains reachable with a sticky bottom treatment
- **AND** it does not cover calibration controls or ignore the device safe-area inset
