# Spec Delta

## ADDED Requirements

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

## MODIFIED Requirements

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
