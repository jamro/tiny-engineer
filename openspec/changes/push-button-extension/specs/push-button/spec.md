# Spec Delta

## Purpose

Lets the user enable one extension-port button that publishes down and up events for host-side automation.

## ADDED Requirements

### Requirement: Push-button defaults off
The setting `push_button` MUST be a boolean on `GET /settings` and `POST /settings`, default **false** when unset, including after factory reset. Saving it MUST apply immediately without reboot.

#### Scenario: Fresh device
- **WHEN** the device boots with no stored `push_button` value
- **THEN** `GET /settings` reports `push_button` false and no button events are published

#### Scenario: Factory reset clears the extension
- **WHEN** factory reset completes
- **THEN** `push_button` is false

#### Scenario: Enable without reboot
- **WHEN** the client posts `{"push_button": true}`
- **THEN** the extension is active before the next reboot

### Requirement: Extensions page owns the toggle
The web UI MUST provide an Extensions page, linked from the main nav, with a control that turns `push_button` on and off. The Config page MUST NOT show this control.

#### Scenario: Toggle from Extensions
- **WHEN** the user turns the push-button control on and saves
- **THEN** the UI posts `push_button` true and the page shows the extension as on

#### Scenario: Config omits the control
- **WHEN** the user opens the Config page
- **THEN** that page has no push-button control

### Requirement: Disabled extension leaves GPIO20 idle
When `push_button` is false, the firmware MUST NOT publish button events and MUST NOT hold GPIO20 as a button input.

#### Scenario: Off ignores the pin
- **WHEN** `push_button` is false and the button contact closes
- **THEN** no button event is published

### Requirement: Enabled extension reads GPIO20 active-low
When `push_button` is true, the firmware MUST read GPIO20 (J4 pin 4) with the internal pull-up enabled. A stable press to ground MUST publish one `button.down` event, and the following stable release MUST publish one `button.up` event. Bounce MUST NOT publish a burst. Enabling the extension MUST sample the current level without publishing; only later edges publish.

#### Scenario: Press and release
- **WHEN** the extension is on and the user presses then releases the button
- **THEN** each outbound sink that can accept the event receives `button.down` followed by `button.up`, each with `v` 1, `source` `push-button`, and `ms`

#### Scenario: Enable while held
- **WHEN** the button is already held and the user turns the extension on
- **THEN** no event is published until the button is released and pressed again

#### Scenario: Disable stops events
- **WHEN** the user sets `push_button` to false while the button is held
- **THEN** no further button events are published, including no synthetic up

### Requirement: Edges during blocking play are kept
A press and release that both occur while `loop()` is blocked by `POST /play` MUST still produce `button.down` then `button.up`, in that order, once the device can publish again.

#### Scenario: Click during play
- **WHEN** the extension is on and the user presses and releases the button during an in-progress `POST /play`
- **THEN** after playback no longer blocks the loop, each outbound sink that can accept the event receives `button.down` then `button.up` for that click
