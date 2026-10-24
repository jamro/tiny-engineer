# push-button Specification

## Purpose

Lets the user enable one extension-port button that publishes down and up events for host-side automation.

## Requirements

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

### Requirement: Extensions page owns the toggle
The web UI MUST provide an Extensions page, linked from the main nav, with a control that turns `push_button` on and off. The Config page MUST NOT show this control. While the draft toggle is off, the page MUST NOT show pin configuration, wiring instructions, or the Events table. While the draft toggle is on, the page MUST show: a pin control limited to GP6, GP7, GP20, and GP21; a short wiring how-to that names the Extension port (not J4) and the selected pin to GND, active-low with internal pull-up; and a compact Events table with one row per outbound button event (`button.down`, `button.up`, `button.click`, `button.double_click`, `button.long_click`) where each row shows the event name as code, a short when-it-fires hint, and a select for sound (`None` / `Bell` / `Click`, values `none` / `bell` / `click`). The page MUST still note that there are no extra HTTP endpoints and that events go out over WebSocket and serial. User-facing UI and docs for this extension MUST say Extension port, not J4.

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
- **THEN** the page shows the pin selector (GP6, GP7, GP20, GP21), wiring how-to naming Extension port, and the Events table with a sound select per event before Save

#### Scenario: Save includes pin
- **WHEN** the toggle is on, the user selects GP7, and saves
- **THEN** the UI posts `push_button` true and `push_button_pin` 7

#### Scenario: Save includes sound mapping
- **WHEN** the toggle is on, the user sets click to Bell and leaves other sounds at None, and saves
- **THEN** the UI posts `push_button_sound_click` `bell` and the other four sound fields as `none`

### Requirement: Per-event sound settings
The five settings `push_button_sound_down`, `_up`, `_click`, `_double_click`, and `_long_click` MUST be strings on `GET`/`POST /settings`, allow only `none`/`bell`/`click`, default to **`none`** when unset (including factory reset), reject other values without saving, persist when `push_button` is false, and apply immediately.

#### Scenario: Fresh device defaults all to none
- **WHEN** the device boots with no stored push-button sound values
- **THEN** `GET /settings` reports all five sound fields as `none`

#### Scenario: Factory reset clears sounds
- **WHEN** factory reset completes
- **THEN** all five sound fields are `none`

#### Scenario: Invalid value rejected
- **WHEN** the client posts `{"push_button_sound_click": "welcome"}`
- **THEN** the device answers 400, nothing is saved, and the previous values remain

#### Scenario: Off keeps saved sounds
- **WHEN** `push_button` is false and the client posts `{"push_button_sound_click": "bell"}`
- **THEN** the value is saved and `GET /settings` reports `push_button_sound_click` `bell` with the extension still off

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

### Requirement: Edges during blocking play are kept within the queue
The extension MUST queue pin edges in a ring of depth **8**. When the ring is full, the oldest edge MUST be dropped. A press and release that both occur while `loop()` is blocked by `POST /play` MUST produce `button.down` then `button.up`, in that order, once the device can publish again, **when both of those edges are still in the queue**. The device MUST NOT promise that guarantee for clicks whose edges were discarded by overflow.

#### Scenario: Click during play retained
- **WHEN** the extension is on and the user presses and releases the button during an in-progress `POST /play` and both edges remain in the queue
- **THEN** after playback no longer blocks the loop, each outbound sink that can accept the event receives `button.down` then `button.up` for that click

#### Scenario: Queue overflow drops oldest edges
- **WHEN** more than eight edges arrive before a poll drains the ring
- **THEN** the oldest edges are discarded and later publishes reflect only the edges that were retained

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

### Requirement: Publish plays mapped sound when free
When the extension publishes a button event and the matching sound setting is `bell` or `click`, the firmware MUST start that LittleFS clip (`/bell.wav` or `/click.wav`) on the speaker without blocking the publish path, unless the speaker is already busy. When the setting is `none`, the firmware MUST NOT start a clip for that event. When the speaker is busy, the firmware MUST skip the clip and MUST still publish the outbound event.

#### Scenario: Click with bell when idle
- **WHEN** `push_button` is true, `push_button_sound_click` is `bell`, the speaker is idle, and a `button.click` is published
- **THEN** sinks receive `button.click` and the device starts playing `/bell.wav`

#### Scenario: None stays silent
- **WHEN** `push_button_sound_down` is `none` and a `button.down` is published
- **THEN** sinks receive `button.down` and no button sound clip starts

#### Scenario: Busy speaker skips sound
- **WHEN** the speaker is already playing (stock WAV stream or `/play`) and a mapped button sound would otherwise start
- **THEN** the outbound event is still published and the button sound clip does not start or interrupt the current playback

### Requirement: Click WAV on filesystem image
The build MUST pack `assets/click.wav` into LittleFS as `/click.wav` (same downsample path as other stock WAVs) so `click` sound settings can play on device.

#### Scenario: Packed click clip exists after buildfs
- **WHEN** the stock filesystem image is built
- **THEN** it contains `/click.wav` derived from `assets/click.wav`
