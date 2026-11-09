# Spec Delta

## MODIFIED Requirements

### Requirement: Extensions page owns the toggle
The web UI MUST provide an Extensions page, linked from the main nav, with a control that turns `push_button` on and off. The Config page MUST NOT show this control. While the draft toggle is off, the page MUST NOT show pin configuration, wiring instructions, or the Events table. While the draft toggle is on, the page MUST show: a pin control limited to GP6, GP7, GP20, and GP21; a short wiring how-to that names the Extension port (not J4) and the selected pin to GND, active-low with internal pull-up; and a compact Events table with one row per outbound button event (`button.down`, `button.up`, `button.click`, `button.double_click`, `button.long_click`) where each row shows the event name as code and a short when-it-fires hint (no per-event sound select). The page MUST note that local reactions are configured on the Actions page, that there are no extra HTTP endpoints for the button itself, and that events go out over WebSocket and serial. User-facing UI and docs for this extension MUST say Extension port, not J4.

#### Scenario: Toggle from Extensions
- **WHEN** the user turns the push-button control on and saves
- **THEN** the UI posts `push_button` true and the page shows the extension as on

#### Scenario: Config omits the control
- **WHEN** the user opens the Config page
- **THEN** that page has no push-button control

#### Scenario: Details hidden when off
- **WHEN** the draft push-button toggle is off
- **THEN** the Extensions page shows no pin selector, no wiring how-to, and no Events table for the push-button

#### Scenario: Details shown when on
- **WHEN** the user turns the draft push-button toggle on
- **THEN** the page shows the pin selector (GP6, GP7, GP20, GP21), wiring how-to naming Extension port, and the Events table without sound selects before Save

#### Scenario: Save includes pin
- **WHEN** the toggle is on, the user selects GP7, and saves
- **THEN** the UI posts `push_button` true and `push_button_pin` 7

#### Scenario: Save includes sound mapping
- **WHEN** the toggle is on and the user saves
- **THEN** the UI does not post any `push_button_sound_*` fields (local sounds live on Actions)

## REMOVED Requirements

### Requirement: Per-event sound settings
**Reason**: Local sound reactions move to the Actions list so one editor covers sound and anim for any event pattern.
**Migration**: On first boot with an empty Actions list, non-`none` `push_button_sound_*` values become sound Action rows; legacy keys are then cleared. Clients must stop reading/writing those settings fields.

### Requirement: Publish plays mapped sound when free
**Reason**: Sound is no longer a push-button side effect; matching Actions handle clips when the event is dispatched.
**Migration**: Configure a sound Action with `on` set to the button event type (or a pattern) and `sound` `bell` or `click`. Speaker busy rules stay the same under Actions.
