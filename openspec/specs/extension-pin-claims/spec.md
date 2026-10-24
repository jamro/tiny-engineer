# extension-pin-claims Specification

## Purpose

Coordinates GPIO claims among enabled Extension-port extensions so two active extensions cannot share the same pin; conflicts are detected at save time, with a safe idle fallback at boot.

## Requirements

### Requirement: Only enabled extensions claim pins
An extension MUST claim its configured GPIO pins only while that extension is enabled. Disabled extensions MUST NOT claim pins, even if their pin settings are stored. New extensions MUST default to disabled so a fresh or factory-reset device has no claims.

#### Scenario: Disabled shares stored pin with no claim
- **WHEN** two extensions store the same pin value and both are disabled
- **THEN** saving those pin values succeeds and neither extension holds the pin as an input

#### Scenario: One enabled claims its pin
- **WHEN** only one extension is enabled and its pin is set
- **THEN** that extension claims the pin and saving succeeds

### Requirement: Save rejects conflicting enabled claims
Before writing settings, the device MUST merge the patch with current values and collect pin claims from every extension that would be enabled. If two or more enabled extensions would claim the same GPIO, the device MUST answer **400**, MUST NOT save any field from that request, and MUST leave runtime claims unchanged. The error message MUST name the conflicting pin and the conflicting extension ids.

#### Scenario: Enabling second extension on same pin fails
- **WHEN** extension A is enabled on GP20 and the client enables extension B also configured for GP20
- **THEN** the device answers 400 naming GP20 and both extensions, and A remains enabled on GP20

#### Scenario: Changing pin onto another enabled claim fails
- **WHEN** A is enabled on GP20, B is enabled on GP6, and the client sets B's pin to 20
- **THEN** the device answers 400 and B remains on GP6

#### Scenario: Partial update of unrelated field with latent conflict still blocked
- **WHEN** A and B are both already enabled on the same pin due to prior corrupt state and the client posts an unrelated settings field
- **THEN** the device answers 400 for the pin conflict and does not save the unrelated field until claims are resolved

### Requirement: Boot idles conflicting claims
At boot (and when applying loaded settings), if two or more enabled extensions claim the same GPIO, the firmware MUST NOT attach any of those conflicting extensions to that pin, MUST leave the conflicting pins idle, MUST keep the stored settings unchanged, and MUST log the conflict.

#### Scenario: Corrupt NVS both enabled same pin
- **WHEN** the device boots with two extensions enabled and both configured for GP20
- **THEN** neither extension drives GP20 as its input, settings still report both enabled with pin 20, and a conflict is logged

#### Scenario: Non-conflicting enabled still applies
- **WHEN** the device boots with one extension enabled on GP20 and another disabled on GP20
- **THEN** the enabled extension attaches to GP20 normally
