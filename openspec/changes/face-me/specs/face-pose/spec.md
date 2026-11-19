# Spec Delta

## Purpose

Lets each user store an optional "face me" pose (head, neck, body) that points the robot at where they sit, so animations that address the user look at them. When the pose is not set, the robot behaves exactly as it does without the feature.

## ADDED Requirements

### Requirement: Face pose is an optional persistent setting
The device MUST store an optional face pose of three values: head, neck and body, in that order, each in normalized pose units from −1 to 1 (−1 = saved min, 0 = mid, 1 = saved max). The pose MUST persist across reboot. A device with no stored pose MUST treat the face pose as not set.

#### Scenario: Fresh device has no face pose
- **WHEN** settings load on a device that never saved a face pose
- **THEN** the face pose is not set

#### Scenario: Saved pose survives reboot
- **WHEN** a client saves face pose `[0.2, -0.4, 0.1]` and the device reboots
- **THEN** the face pose is `[0.2, -0.4, 0.1]`

### Requirement: Settings API exposes face pose
`GET /settings` MUST include `face_pose`: a three-number array `[head, neck, body]` when set, or an empty array when not set. `POST /settings` MUST accept `face_pose` as three numbers from −1 to 1 to set it, or as an empty array to clear it, in normal mode and in setup AP mode.

#### Scenario: Read when not set
- **WHEN** a client sends `GET /settings` and no face pose is set
- **THEN** the response contains `"face_pose": []`

#### Scenario: Set the pose
- **WHEN** a provisioned client sends `POST /settings` with `{"face_pose":[0.15,-0.3,0.1]}`
- **THEN** the device answers success and the next `GET /settings` returns `"face_pose": [0.15,-0.3,0.1]`

#### Scenario: Clear the pose
- **WHEN** a client sends `POST /settings` with `{"face_pose":[]}`
- **THEN** the face pose is not set and `GET /settings` returns `"face_pose": []`

#### Scenario: Set in setup AP mode
- **WHEN** setup AP mode is active and a client sends `POST /settings` with a valid `face_pose`
- **THEN** the device saves it and does not answer `setup only in AP mode` or any other mode error

### Requirement: Invalid face pose is rejected without change
The device MUST answer `400` with `invalid face_pose` and MUST NOT change any setting from that request when `face_pose` is not an array, has a length other than 0 or 3, contains a non-number, or contains a value outside −1..1.

#### Scenario: Value out of range
- **WHEN** a client sends `POST /settings` with `{"face_pose":[0,1.5,0],"volume":40}`
- **THEN** the device answers `400` `invalid face_pose`
- **AND** neither the face pose nor the volume changes

#### Scenario: Wrong length
- **WHEN** a client sends `POST /settings` with `{"face_pose":[0,0]}`
- **THEN** the device answers `400` `invalid face_pose`

#### Scenario: Null is a wrong type
- **WHEN** a client sends `POST /settings` with `{"face_pose":null}`
- **THEN** the device answers `400` `invalid face_pose`

### Requirement: Factory reset clears face pose
A factory reset MUST clear the face pose, because it depends on the desk setup and not on the robot build.

#### Scenario: Reset then read
- **WHEN** a face pose is set and the user performs a factory reset
- **THEN** after reset `GET /settings` returns `"face_pose": []`
- **AND** the saved servo ranges are unchanged

### Requirement: Talking faces the user when face pose is set
When the face pose is set, the talking animation MUST use the face head, neck and body as its rest pose, and MUST apply its head nods, neck drift and body sway as offsets from that rest pose, limited to −1..1. A change to the face pose MUST apply the next time talking starts, without reboot.

#### Scenario: Talking settles on the face pose
- **WHEN** the face pose is `[0.3, -0.5, 0.2]` and talking starts with no audio
- **THEN** the head, neck and body move toward `0.3`, `-0.5` and `0.2`
- **AND** later neck drift and body sway stay within their normal sway width around `-0.5` and `0.2`

#### Scenario: Offsets are limited
- **WHEN** the face neck is `1` and talking drifts the neck to a positive offset
- **THEN** the neck target is `1`, not a value above the saved max

#### Scenario: New pose applies without reboot
- **WHEN** a client saves a new face pose and then starts talking
- **THEN** talking uses the new face pose

### Requirement: No face pose keeps current motion
When the face pose is not set, every animation MUST use the same poses it uses without this feature. Talking MUST use head `1/7`, neck `0` and body `0` as its rest pose.

#### Scenario: Talking without face pose
- **WHEN** no face pose is set and talking starts
- **THEN** talking rests at head `1/7`, neck `0` and body `0`, as before this change

#### Scenario: Other animations ignore the face pose
- **WHEN** a face pose is set and typing, reading, thinking, welcome, attention or ring plays
- **THEN** those animations use the same poses as when no face pose is set

### Requirement: Web UI calibrates the face pose
After provisioning, the Servo control page of the web UI MUST provide a Face me section that moves head, neck and body one at a time inside their saved ranges with nudge steps of ±1° and ±5° in whole degrees, shows the selected joint on a bar that spans its saved min to max, saves the current pose as the face pose, clears it, and plays the talking animation as a preview. The section MUST show whether a face pose is set.

#### Scenario: Start from the saved or default pose
- **WHEN** the user opens the Servo control page
- **THEN** the controls start at the saved face pose, or at head `1/7`, neck `0`, body `0` when none is set
- **AND** the Face me section states whether a face pose is set

#### Scenario: Adjust a joint
- **WHEN** the user picks head, neck or body and chooses a ±1° or ±5° nudge
- **THEN** the robot moves that joint to the shown whole-degree angle plus that step, limited to its saved range
- **AND** the bar marker moves to the new angle between the saved min and max
- **AND** the stored value is the matching −1..1 pose unit

#### Scenario: Save
- **WHEN** the user chooses Save
- **THEN** the UI sends the three current values as `face_pose` via `POST /settings`

#### Scenario: Clear
- **WHEN** the user chooses Clear and confirms
- **THEN** the UI sends `"face_pose": []` and the section shows that no face pose is set

#### Scenario: Preview
- **WHEN** the user chooses Preview
- **THEN** the robot plays the talking animation

#### Scenario: Recalibration notice
- **WHEN** the Face me section shows a set face pose
- **THEN** the section tells the user to check the face pose again after a servo range calibration

### Requirement: Setup wizard offers an optional Face me step
The setup AP wizard MUST show a Face me step directly after the servos step. The step MUST let the user move head, neck and body and save the face pose, and MUST offer Skip. Skip MUST advance without changing the face pose.

#### Scenario: Step order
- **WHEN** the user leaves the servos step
- **THEN** the next wizard step is Face me

#### Scenario: Skip keeps the current value
- **WHEN** the user chooses Skip on the Face me step
- **THEN** the wizard advances to the next step
- **AND** no `face_pose` is sent

#### Scenario: Save and continue
- **WHEN** the user moves the joints and chooses Save and continue
- **THEN** the UI sends the three current values as `face_pose` via `POST /settings` and advances

#### Scenario: Wizard moves use the setup move
- **WHEN** the user nudges a joint on the Face me step
- **THEN** the UI uses the setup servo move endpoint with the matching angle inside the saved range
