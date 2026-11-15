# Spec Delta

## Purpose

Lets the robot run on-device Actions when outbound events fire: bind typed event patterns to sound or animation effects, manage them over HTTP and the Actions web UI, and simulate events for testing without a host.

## ADDED Requirements

### Requirement: Action row shape
Each Action MUST be one object with string `on` (event pattern), string `do` (`sound` or `anim`), and type-specific params. `sound` MUST set `sound` to `bell` or `click`. `anim` MUST set `anim` to a robot animation name. Extra fields MUST be ignored. Invalid rows MUST be rejected on save.

#### Scenario: Valid sound row
- **WHEN** the client saves a row `{ "on": "button.click", "do": "sound", "sound": "bell" }`
- **THEN** the list stores that row and later loads return it

#### Scenario: Invalid do rejected
- **WHEN** the client saves a row with `do` `script`
- **THEN** the device answers 400 and the previous list remains

#### Scenario: Sound missing sound field
- **WHEN** the client saves `{ "on": "button.click", "do": "sound" }` without `sound`
- **THEN** the device answers 400 and nothing is saved

### Requirement: Replace-all Actions list API
The device MUST expose `GET /settings/actions` returning `{ "actions": [ ... ] }` in stored order, and `PUT /settings/actions` with a JSON object containing array `actions` that replaces the whole list after validation. An empty array MUST clear all Actions. Partial update MUST NOT be supported. Access MUST match other settings list routes (Bearer when a token is set). The SPA page GET `/actions` MUST serve the web UI shell and MUST NOT return the JSON list.

#### Scenario: Empty list
- **WHEN** no Actions are stored
- **THEN** `GET /settings/actions` returns `{ "actions": [] }`

#### Scenario: Replace all
- **WHEN** the client PUTs two valid rows to `/settings/actions`
- **THEN** `GET /settings/actions` returns exactly those two rows in the same order

#### Scenario: Over max rejected
- **WHEN** the client PUTs more than **16** rows to `/settings/actions`
- **THEN** the device answers 400 and the previous list remains

### Requirement: Pattern matching with wildcards
`on` MUST be a non-empty pattern of at most **47** characters. `*` MUST match any sequence of characters (including empty) and MAY appear any number of times in any position. Matching MUST compare against the event `type` string only (not `source`). A row whose pattern matches MUST be eligible to run.

#### Scenario: Exact match
- **WHEN** an event type `button.click` is dispatched and a row has `on` `button.click`
- **THEN** that row runs

#### Scenario: Suffix wildcard
- **WHEN** an event type `button.double_click` is dispatched and a row has `on` `button.*`
- **THEN** that row runs

#### Scenario: Mid wildcard
- **WHEN** an event type `button.double_click` is dispatched and a row has `on` `*.double_*`
- **THEN** that row runs

#### Scenario: Non-match skipped
- **WHEN** an event type `button.up` is dispatched and a row has `on` `button.click`
- **THEN** that row does not run

### Requirement: Fire all matching Actions
When an event type is dispatched to Actions, the firmware MUST evaluate every stored row in list order and MUST run every row whose `on` pattern matches. One mismatch MUST NOT stop later matches.

#### Scenario: Two rows same event
- **WHEN** two rows both match `button.click` (sound then anim) and `button.click` is dispatched
- **THEN** both the sound and the anim effects run, in list order

#### Scenario: Mix of match and miss
- **WHEN** rows exist for `button.click` and `button.up` and `button.click` is dispatched
- **THEN** only the `button.click` row runs

### Requirement: Sound action plays clip
A matching `sound` Action MUST start `/bell.wav` or `/click.wav` with the same speaker busy rules as today’s button SFX: skip when `/play` holds the speaker or a sound-owning animation is streaming; preempt an orphan non-animation stream; never block the publish path.

#### Scenario: Bell when idle
- **WHEN** a matching sound Action with `sound` `bell` runs and the speaker is free
- **THEN** the device starts `/bell.wav`

#### Scenario: Busy speaker skips
- **WHEN** `/play` holds the speaker busy and a matching sound Action would otherwise start
- **THEN** the sound clip does not start

### Requirement: Anim action uses API path
A matching `anim` Action MUST trigger the named animation through the same path and rules as `POST /anim` (same name validation and busy/replace behavior). An unknown anim name at run time MUST be skipped without affecting other matching Actions.

#### Scenario: Welcome anim
- **WHEN** a matching Action `{ "do": "anim", "anim": "welcome" }` runs
- **THEN** the robot starts the welcome animation the same way as `POST /anim?name=welcome`

#### Scenario: Unknown anim skipped
- **WHEN** a matching Action names an unknown animation
- **THEN** that Action is skipped and other matching Actions still run

### Requirement: Dispatch on published events
Whenever the firmware publishes a discrete outbound event envelope, it MUST also dispatch that event’s `type` to Actions after (or as part of) publish, so host sinks and on-device Actions both see the same logical event. Actions MUST NOT themselves publish a new outbound event for the same dispatch.

#### Scenario: Button click runs Actions and sinks
- **WHEN** the push-button extension publishes `button.click`
- **THEN** outbound sinks receive the envelope and matching Actions run

### Requirement: Simulate event for testing
The device MUST expose `POST /actions/test` with a JSON object body containing non-empty string `type`. It MUST dispatch that `type` through Actions matching (fire all matches) and MUST NOT publish an outbound event envelope for the simulation. Access MUST require configured robot + Bearer when a token is set (same class as `POST /anim`). Missing or empty `type` MUST answer 400.

#### Scenario: Simulate matching pattern
- **WHEN** a row has `on` `button.*` with `do` `sound` `click`, and the client posts `{ "type": "button.double_click" }` to `/actions/test`
- **THEN** the click sound starts (if the speaker rules allow) and no `button.double_click` envelope is sent on WebSocket or serial

#### Scenario: Missing type rejected
- **WHEN** the client posts `{}` to `/actions/test`
- **THEN** the device answers 400 and no Actions run

### Requirement: Actions web UI tab
The web UI MUST provide an Actions page at `/actions`, linked from the main nav as **Actions**. The Config page MUST NOT own this editor. Save MUST use `PUT /settings/actions`. Load MUST use `GET /settings/actions`.

#### Scenario: Nav link
- **WHEN** the user opens the robot web UI outside setup mode
- **THEN** the main nav includes Actions linking to `/actions`

#### Scenario: Save replace-all
- **WHEN** the user adds two rows and saves
- **THEN** the UI PUTs the full `actions` array to `/settings/actions` and shows the saved list after success

### Requirement: Actions row editor controls
The Actions page MUST let the user add, edit, and remove rows with a text field for the event pattern, a control for action kind, and kind-specific params: sound select (`bell`/`click`) and anim select using the same names as the Animations page.

#### Scenario: Kind-specific params
- **WHEN** the user sets a row kind to anim
- **THEN** the row shows an anim select and does not show a sound select

### Requirement: Actions simulate control
The Actions page MUST include a simulate-event control (text input + trigger) that calls `POST /actions/test` with the entered type.

#### Scenario: Simulate from UI
- **WHEN** the user enters `button.click` in the simulate field and triggers it
- **THEN** the UI posts `{ "type": "button.click" }` to `/actions/test`

### Requirement: Legacy button sounds migrate into Actions
On load, when the Actions list is empty and any legacy `push_button_sound_*` NVS value is `bell` or `click`, the firmware MUST create one sound Action per non-`none` mapping (`on` = that button event type), persist the Actions list, and clear the legacy sound keys only after a successful write. When Actions already has rows, legacy keys MUST be cleared without creating rows. Factory reset MUST leave Actions empty.

#### Scenario: Migrate click bell
- **WHEN** Actions is empty and legacy click sound is `bell` with others `none`
- **THEN** after load, Actions contains one row `on` `button.click` `do` `sound` `sound` `bell`, and legacy sound keys are gone

#### Scenario: Existing Actions wins
- **WHEN** Actions already has rows and legacy sound keys are still present
- **THEN** load keeps the existing Actions and clears the legacy keys without adding migrated rows
