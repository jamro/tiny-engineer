# Spec Delta

## MODIFIED Requirements

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
- **AND** an internal progress indicator advances independently of the outer wizard progress
