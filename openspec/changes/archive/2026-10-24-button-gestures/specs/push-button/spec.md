# Spec Delta

## ADDED Requirements

### Requirement: Gesture events accompany raw edges
When `push_button` is true, the firmware MUST keep publishing `button.down` and `button.up` on debounced edges as today, and MUST also publish semantic gesture events `button.click`, `button.double_click`, and `button.long_click` according to the timing rules below. Every gesture event MUST use envelope `v` 1, `source` `push-button`, and `ms`. Thresholds MUST be fixed in firmware (not settings): long hold **800 ms**, double-click wait **350 ms** after a short release.

#### Scenario: Short press yields delayed click
- **WHEN** the extension is on and the user presses and releases with hold shorter than 800 ms and does not press again within 350 ms
- **THEN** sinks receive `button.down`, then `button.up`, then after the 350 ms wait one `button.click`

#### Scenario: Double press yields double_click only
- **WHEN** the extension is on and the user completes two short presses whose second down falls within 350 ms of the first short up
- **THEN** sinks receive the corresponding `button.down` / `button.up` pairs and one `button.double_click`, and MUST NOT receive `button.click` for that pair

### Requirement: Long click fires on hold
When the button stays pressed for at least **800 ms** after a debounced `button.down`, the firmware MUST publish one `button.long_click` while the button is still held. A later release MUST publish `button.up` and MUST NOT publish `button.click` or `button.double_click` for that press.

#### Scenario: Hold then release
- **WHEN** the extension is on and the user holds the button for at least 800 ms then releases
- **THEN** sinks receive `button.down`, then `button.long_click` before release, then `button.up`, and no `button.click` or `button.double_click`

#### Scenario: Second press becomes long
- **WHEN** a short press was released and a second press starts within the 350 ms window then is held for at least 800 ms
- **THEN** sinks receive `button.long_click` for the second press and MUST NOT receive `button.click` or `button.double_click` for the first short press

### Requirement: Gesture timeouts advance without new edges
While the extension is on, the firmware MUST evaluate long-hold and delayed-click deadlines on each poll using current time, including polls during `POST /play` body handling, so a long hold or a pending click can publish even when no new pin edge arrives.

#### Scenario: Long click during blocking play
- **WHEN** the button is held across a poll during an in-progress `POST /play` and hold time reaches 800 ms
- **THEN** `button.long_click` is published when that poll runs, without waiting for release

#### Scenario: Click timeout during idle loop
- **WHEN** a short press was released and 350 ms pass with no second press
- **THEN** `button.click` is published on a subsequent poll after that deadline
