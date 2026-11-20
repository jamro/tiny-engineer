# Spec Delta

## ADDED Requirements

### Requirement: Attention faces the user when face pose is set
When the face pose is set, the attention animation MUST use the face head, neck and body as its rest pose for prep, audio motion and the waiting loop, and MUST apply its existing neck glances, head nods and wait drifts as offsets from that rest pose, limited to −1..1. Hand poses MUST stay as they are without this feature. A change to the face pose MUST apply the next time attention starts, without reboot.

#### Scenario: Attention settles on the face pose
- **WHEN** the face pose is `[0.3, -0.5, 0.2]` and attention starts
- **THEN** the prep pose moves head, neck and body toward `0.3`, `-0.5` and `0.2`
- **AND** later audio glances and wait drifts stay within their normal offset width around that rest pose

#### Scenario: Attention offsets are limited
- **WHEN** the face neck is `1` and attention applies a positive neck glance
- **THEN** the neck target is `1`, not a value above the saved max

#### Scenario: New face pose applies on next attention
- **WHEN** a client saves a new face pose and then starts attention
- **THEN** attention uses the new face pose

### Requirement: Attention without face pose keeps stock motion
When the face pose is not set, attention MUST use the same poses it uses without this feature: head `6/35`, neck `0` and body `0` as its rest pose.

#### Scenario: Attention without face pose
- **WHEN** no face pose is set and attention starts
- **THEN** attention rests at head `6/35`, neck `0` and body `0`, as before this change
