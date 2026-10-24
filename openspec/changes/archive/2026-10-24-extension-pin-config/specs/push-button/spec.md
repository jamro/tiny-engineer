# Spec Delta

## ADDED Requirements

### Requirement: Push-button pin setting
The setting `push_button_pin` MUST be an integer on `GET /settings` and `POST /settings`, default **20** when unset, including after factory reset. Allowed values MUST be only **6**, **7**, **20**, and **21**. Values outside that set MUST be rejected without saving. Saving a valid pin MUST persist even when `push_button` is false. Saving MUST apply immediately without reboot when the extension is on.

#### Scenario: Fresh device defaults to GP20
- **WHEN** the device boots with no stored `push_button_pin` value
- **THEN** `GET /settings` reports `push_button_pin` 20

#### Scenario: Factory reset restores default pin
- **WHEN** factory reset completes
- **THEN** `push_button_pin` is 20

#### Scenario: Off keeps the saved pin
- **WHEN** `push_button` is false and the client posts `{"push_button_pin": 6}`
- **THEN** the value is saved and `GET /settings` reports `push_button_pin` 6 with the extension still off

#### Scenario: Invalid pin rejected
- **WHEN** the client posts `{"push_button_pin": 8}`
- **THEN** the device answers 400, nothing is saved, and the previous pin remains

#### Scenario: Pin change while enabled rebinds without reboot
- **WHEN** `push_button` is true and the client posts a different allowed `push_button_pin`
- **THEN** the extension reads the new pin before the next reboot

### Requirement: Disabled extension leaves configured pin idle
When `push_button` is false, the firmware MUST NOT publish button events and MUST NOT hold the configured `push_button_pin` as a button input.

#### Scenario: Off ignores the pin
- **WHEN** `push_button` is false and the button contact closes on the configured pin
- **THEN** no button event is published

### Requirement: Enabled extension reads configured pin active-low
When `push_button` is true, the firmware MUST read the configured `push_button_pin` on the Extension port with the internal pull-up enabled. A stable press to ground MUST publish one `button.down` event, and the following stable release MUST publish one `button.up` event. Bounce MUST NOT publish a burst. Enabling the extension MUST sample the current level without publishing; only later edges publish.

#### Scenario: Press and release
- **WHEN** the extension is on and the user presses then releases the button
- **THEN** each outbound sink that can accept the event receives `button.down` followed by `button.up`, each with `v` 1, `source` `push-button`, and `ms`

#### Scenario: Enable while held
- **WHEN** the button is already held and the user turns the extension on
- **THEN** no event is published until the button is released and pressed again

#### Scenario: Disable stops events
- **WHEN** the user sets `push_button` to false while the button is held
- **THEN** no further button events are published, including no synthetic up

#### Scenario: Non-default pin
- **WHEN** `push_button` is true, `push_button_pin` is 6, and the user presses then releases on GP6
- **THEN** sinks receive `button.down` then `button.up` as for the default pin

## MODIFIED Requirements

### Requirement: Extensions page owns the toggle
The web UI MUST provide an Extensions page, linked from the main nav, with a control that turns `push_button` on and off. The Config page MUST NOT show this control. While the draft toggle is off, the page MUST NOT show pin configuration, wiring instructions, or the extension API section. While the draft toggle is on, the page MUST show: a pin control limited to GP6, GP7, GP20, and GP21; a short wiring how-to that names the Extension port (not J4) and the selected pin to GND, active-low with internal pull-up; and an API section listing the outbound button events (and that there are no extra HTTP endpoints for this extension). User-facing UI and docs for this extension MUST say Extension port, not J4.

#### Scenario: Toggle from Extensions
- **WHEN** the user turns the push-button control on and saves
- **THEN** the UI posts `push_button` true and the page shows the extension as on

#### Scenario: Config omits the control
- **WHEN** the user opens the Config page
- **THEN** that page has no push-button control

#### Scenario: Details hidden when off
- **WHEN** the draft push-button toggle is off
- **THEN** the Extensions page shows no pin selector, no wiring how-to, and no API section for the push-button

#### Scenario: Details shown when on
- **WHEN** the user turns the draft push-button toggle on
- **THEN** the page shows the pin selector (GP6, GP7, GP20, GP21), wiring how-to naming Extension port, and an API section for the button events before Save

#### Scenario: Save includes pin
- **WHEN** the toggle is on, the user selects GP7, and saves
- **THEN** the UI posts `push_button` true and `push_button_pin` 7

## REMOVED Requirements

### Requirement: Disabled extension leaves GPIO20 idle
**Reason**: Pin is configurable via `push_button_pin`; idle behavior applies to the configured pin.
**Migration**: Use requirement "Disabled extension leaves configured pin idle".

### Requirement: Enabled extension reads GPIO20 active-low
**Reason**: Pin is configurable; user-facing text uses Extension port instead of J4.
**Migration**: Use requirement "Enabled extension reads configured pin active-low".
