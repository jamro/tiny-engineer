# Spec Delta

## ADDED Requirements

### Requirement: Welcome hello setting
The setting `lcd1602_welcome_hello` MUST be a boolean on `GET`/`POST /settings`, default **false** when unset, including after factory reset. It MUST persist when `lcd1602` is false and apply immediately. When false, welcome animations MUST NOT change LCD text for this feature.

#### Scenario: Default false
- **WHEN** the device has no stored `lcd1602_welcome_hello`
- **THEN** `GET /settings` reports `lcd1602_welcome_hello` false

#### Scenario: Flag off leaves LCD text alone
- **WHEN** `lcd1602` is true, `lcd1602_welcome_hello` is false, and a `welcome` animation runs
- **THEN** the LCD text is not replaced with HELLO/HUMAN by this feature

### Requirement: Welcome hello overlay
When `lcd1602` and `lcd1602_welcome_hello` are true, every `welcome` animation (boot after Wi‑Fi and `POST /anim` `welcome`) MUST show line1 **HELLO** and line2 **HUMAN**, both center-aligned, for the animation duration. HELLO/HUMAN MUST NOT be written to last-state NVS. When welcome ends with no queued text update, the firmware MUST restore the pre-overlay logical text on the display.

#### Scenario: Boot welcome shows hello
- **WHEN** the extension is on, the flag is true, and boot starts `welcome` after Wi‑Fi connect
- **THEN** the LCD shows centered HELLO and HUMAN until that welcome finishes

#### Scenario: API welcome shows hello
- **WHEN** the extension is on, the flag is true, and the client posts `welcome` via `/anim`
- **THEN** the LCD shows centered HELLO and HUMAN until that welcome finishes

#### Scenario: Restore after welcome
- **WHEN** welcome ends and no text/clear was queued during the overlay
- **THEN** the display shows the logical text from before the overlay

### Requirement: Queue text during welcome hello
While the welcome-hello overlay is active, successful `POST`/`DELETE` text (HTTP or serial `ext`) MUST NOT change the visible HELLO/HUMAN lines, MUST NOT persist last state for those queued edits until applied, MUST answer **200**, and MUST last-wins queue the resulting logical text. When welcome ends, the firmware MUST apply the queued logical text to the display and persist last state. Brightness and color MUST still apply immediately.

#### Scenario: Post during overlay queues
- **WHEN** the overlay is active and the client posts new line text
- **THEN** the response is **200**, HELLO/HUMAN stay visible, and after welcome ends the display shows the posted text

#### Scenario: Last wins
- **WHEN** two successful text updates arrive during one overlay
- **THEN** after welcome ends the display matches the later update

#### Scenario: Delete during overlay queues clear
- **WHEN** the overlay is active and the client deletes text
- **THEN** HELLO/HUMAN stay visible until welcome ends, then both lines are empty

### Requirement: GET text is logical during welcome hello
While the welcome-hello overlay is active, `GET /extensions/lcd1602/text` (and serial `ext` GET text) MUST return the logical text: the queued result if a text/clear was accepted during the overlay, otherwise the pre-overlay stash. It MUST NOT return HELLO/HUMAN as the logical lines.

#### Scenario: Get during overlay without queue
- **WHEN** the overlay is active, no text was queued, and the client GETs text
- **THEN** the body matches the pre-overlay logical lines, not HELLO/HUMAN

#### Scenario: Get during overlay with queue
- **WHEN** the overlay is active, a text update was queued, and the client GETs text
- **THEN** the body matches the queued logical lines, not HELLO/HUMAN

## MODIFIED Requirements

### Requirement: Extensions page owns LCD 1602 controls
The Extensions page MUST include an **LCD 1602** section for `lcd1602` and related boot settings. The Config page MUST NOT show these controls. Details and wiring MUST appear only while the draft toggle is on; the section MUST note `/extensions/lcd1602/` HTTP paths and that this extension emits no events.

#### Scenario: Toggle from Extensions
- **WHEN** the user enables LCD 1602 and saves
- **THEN** the UI posts `lcd1602` true

#### Scenario: Details hidden when off
- **WHEN** the draft LCD toggle is off
- **THEN** LCD boot/backlight/line controls and LCD wiring are not shown

#### Scenario: Details shown when on
- **WHEN** the draft LCD toggle is on
- **THEN** the section shows a **Follow sleep** backlight toggle bound to `lcd1602_backlight_sleep`, a **Hello on welcome** toggle bound to `lcd1602_welcome_hello`, boot-content mode, Waveshare I2C Extension-port wiring, and (when mode is `config`) the two line fields

#### Scenario: Config omits LCD
- **WHEN** the user opens the Config page
- **THEN** that page has no LCD 1602 controls
