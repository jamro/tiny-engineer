# lcd1602 Specification

## Purpose
Optional Waveshare LCD1602 I2C extension that shows short status text over REST when enabled from the Extensions page, with boot content and brightness control.

## Requirements

### Requirement: LCD 1602 defaults off
The setting `lcd1602` MUST be a boolean on `GET /settings` and `POST /settings`, default **false** when unset, including after factory reset. Saving it MUST apply immediately without reboot.

#### Scenario: Fresh device
- **WHEN** the device boots with no stored `lcd1602` value
- **THEN** `GET /settings` reports `lcd1602` false and the firmware does not initialize the LCD over I2C

#### Scenario: Factory reset
- **WHEN** factory reset completes
- **THEN** `lcd1602` is false

#### Scenario: Enable without reboot
- **WHEN** the client posts `{"lcd1602": true}`
- **THEN** the extension becomes active before the next reboot

### Requirement: Disabled extension leaves LCD bus idle
When `lcd1602` is false, the firmware MUST NOT probe, initialize, write, or read the LCD or its backlight controller over I2C for this extension, and MUST NOT run LCD scroll ticks.

#### Scenario: Module unplugged while off
- **WHEN** `lcd1602` is false and no LCD is connected
- **THEN** boot and runtime continue without LCD I2C traffic from this extension

#### Scenario: Disable stops scroll
- **WHEN** the extension was scrolling and the client posts `{"lcd1602": false}`
- **THEN** scroll ticks stop and further LCD I2C writes for this extension do not run

### Requirement: Fixed Waveshare I2C addresses
When enabled, the extension MUST use LCD address **0x3E** (AiP31068L). Controllable backlight MUST auto-detect **0x6B** (SN3193, Blue/Gray/Green) or **0x60** (PCA9633, RGB module); those addresses MUST NOT be settings. On PCA9633, brightness MUST scale the base RGB color; the color API sets that base (default white 255,255,255). The extension MUST target the Waveshare LCD1602 I2C module family, not PCF8574 backpacks.

#### Scenario: No address settings
- **WHEN** a client posts an unknown field intended as an LCD I2C address
- **THEN** `POST /settings` rejects it as an unknown or invalid field and does not save it

### Requirement: Follow sleep backlight setting
The setting `lcd1602_backlight_sleep` MUST be a boolean on `GET`/`POST /settings`, default **true** when unset, including after factory reset. It MUST persist when `lcd1602` is false. On enable/boot with the extension on, when boot content is not restoring brightness from `last`, the firmware MUST turn the backlight on (default brightness). When the setting is false, enable/boot MUST still turn the backlight on even if `last` brightness is 0.

#### Scenario: Default true
- **WHEN** the device has no stored `lcd1602_backlight_sleep`
- **THEN** `GET /settings` reports `lcd1602_backlight_sleep` true

#### Scenario: Boot on when follow sleep off
- **WHEN** `lcd1602` is true, `lcd1602_backlight_sleep` is false, and boot content does not restore brightness from `last`
- **THEN** the backlight turns on at enable/boot

#### Scenario: Boot on when follow sleep off and last brightness is zero
- **WHEN** `lcd1602` is true, `lcd1602_backlight_sleep` is false, `lcd1602_boot` is `last`, and stored last brightness is 0
- **THEN** the backlight turns on at enable/boot (does not stay at 0)

#### Scenario: Old key not restored
- **WHEN** the client posts `{"lcd1602_backlight_boot": true}`
- **THEN** `GET /settings` does not include `lcd1602_backlight_boot`

### Requirement: Sleep and wake backlight fades
When the extension is on and `lcd1602_backlight_sleep` is true, entering sleep (`sleep` anim or idle `sleep_timeout`) MUST fade backlight to **0**, and wakeup / wake-from-sleep MUST fade backlight to **100**, over the same nod-off duration used for sleep motion. When `lcd1602_backlight_sleep` is false, sleep and wake MUST NOT change backlight. Manual brightness API MUST still apply regardless of the flag.

#### Scenario: Follow sleep dims on sleep
- **WHEN** `lcd1602` is true, `lcd1602_backlight_sleep` is true, and the device enters sleep
- **THEN** the backlight fades to 0

#### Scenario: Follow sleep lights on wake
- **WHEN** `lcd1602` is true, `lcd1602_backlight_sleep` is true, and the device wakes or plays `wakeup`
- **THEN** the backlight fades to 100

#### Scenario: Follow sleep off leaves backlight
- **WHEN** `lcd1602` is true, `lcd1602_backlight_sleep` is false, and the device sleeps then wakes
- **THEN** backlight brightness is unchanged by those transitions

### Requirement: Boot content mode
The setting `lcd1602_boot` MUST be a string on `GET`/`POST /settings` allowing only `none`, `last`, and `config`, default **`none`** when unset, including after factory reset. Invalid values MUST be rejected without saving. The value MUST persist when `lcd1602` is false. On enable/boot with the extension on, the firmware MUST apply that mode.

#### Scenario: none clears
- **WHEN** `lcd1602_boot` is `none` and the extension becomes active
- **THEN** both lines are cleared (empty) on the display when hardware is present

#### Scenario: last restores
- **WHEN** `lcd1602_boot` is `last`, a last state is stored, and the extension becomes active with LCD present
- **THEN** the display shows the stored lines, aligns, and brightness

#### Scenario: config shows settings lines
- **WHEN** `lcd1602_boot` is `config` and the extension becomes active with LCD present
- **THEN** the display shows `lcd1602_line1` and `lcd1602_line2` with default left align unless those strings are empty

#### Scenario: invalid boot mode
- **WHEN** the client posts `{"lcd1602_boot": "welcome"}`
- **THEN** the device answers **400**, nothing is saved

### Requirement: Boot config line strings
The settings `lcd1602_line1` and `lcd1602_line2` MUST be strings on `GET`/`POST /settings`, default empty when unset, including after factory reset. Each MUST accept at most **64** characters; longer values MUST be rejected without saving. They MUST persist when `lcd1602` is false and apply when `lcd1602_boot` is `config`.

#### Scenario: Persist while off
- **WHEN** `lcd1602` is false and the client posts config lines
- **THEN** the values are saved and `GET /settings` returns them

#### Scenario: Too long rejected
- **WHEN** the client posts a line longer than 64 characters
- **THEN** the device answers **400** and nothing is saved

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

### Requirement: Text API partial update
`POST /extensions/lcd1602/text` MUST accept JSON where omitted `line1`/`line2` leave that line unchanged, present line objects carry required `text` (0–64 chars) and optional `align` (default `left`), empty `text` clears the line, bad input is **400**, and success updates display when present, persists last state, and returns **200** with full text state.

#### Scenario: Update one line
- **WHEN** only `line1` is posted with `{"text":"HI"}`
- **THEN** line1 becomes `HI` (align left) and line2 is unchanged

#### Scenario: Clear one line
- **WHEN** the client posts `{"line1":{"text":""}}`
- **THEN** line1 is cleared and line2 is unchanged

#### Scenario: Align center under 16
- **WHEN** the client posts `{"line1":{"text":"OK","align":"center"}}` and the text length is ≤16
- **THEN** the visible line is centered in 16 columns

### Requirement: Long line marquees LTR
When a line’s `text` length is greater than 16, the firmware MUST ignore `align` for that line and scroll a 16-character window `[i .. i+16)` advancing `i` by one on each step (wrapping), at a fixed readable pace. Lines ≤16 MUST NOT marquee.

#### Scenario: Long line scrolls
- **WHEN** line1 text has more than 16 characters and the extension is on with LCD present
- **THEN** the visible 16 columns advance through the string in a loop until the line is replaced or cleared

#### Scenario: Short line static
- **WHEN** line1 text has 16 or fewer characters with align `right`
- **THEN** the line stays static and right-aligned

### Requirement: GET text returns full logical state
`GET /extensions/lcd1602/text` MUST return JSON with `line1` and `line2` objects each containing `text` and `align` for the current logical content (full string, not only the 16-column viewport).

#### Scenario: Get after partial post
- **WHEN** the client successfully set both lines then `GET`s `/extensions/lcd1602/text`
- **THEN** the body includes both lines’ full `text` and `align` values

### Requirement: DELETE text clears both lines
`DELETE /extensions/lcd1602/text` MUST clear both lines, stop marquees, leave brightness unchanged, persist last state, and answer **200** with the cleared text state when the extension is on.

#### Scenario: Clear both
- **WHEN** the client `DELETE`s `/extensions/lcd1602/text` while the extension is on
- **THEN** both lines are empty and brightness is unchanged

### Requirement: Brightness API
`GET /extensions/lcd1602/brightness` MUST return JSON `{"brightness": <0-100>}`. `POST /extensions/lcd1602/brightness` MUST accept `{"brightness": <0-100>}`, apply backlight (0 = off), persist last state, and answer **200** with the new value. On PCA9633, brightness MUST scale the stored base RGB color (`PWM = channel × brightness / 100`). Out-of-range or bad types MUST answer **400** without changing brightness.

#### Scenario: Set brightness
- **WHEN** the client posts `{"brightness": 50}` while the extension is on and LCD backlight is reachable
- **THEN** backlight is set near 50% and `GET` reports `50`

#### Scenario: Invalid brightness
- **WHEN** the client posts `{"brightness": 101}`
- **THEN** the response is **400** and brightness is unchanged

### Requirement: RGB color API
`GET` / `POST /extensions/lcd1602/color` MUST be available when the detected backlight is PCA9633. `GET` MUST return `{"r","g","b"}` each 0–255. `POST` MUST accept those three fields, store them as the base color, apply PWM scaled by current brightness, persist last state, and answer **200**. When the backlight is SN3193, color MUST answer **400** `rgb not supported`. When no backlight chip ACKs, color MUST answer **503** `backlight not found`.

#### Scenario: Set color on RGB module
- **WHEN** the PCA9633 ACKs and the client posts `{"r":0,"g":128,"b":255}` while the extension is on
- **THEN** the response is **200** with that color and the backlight uses those channels scaled by brightness

#### Scenario: Color rejected on SN3193
- **WHEN** the SN3193 ACKs and the client posts color
- **THEN** the response is **400** with `rgb not supported`

#### Scenario: Color without backlight
- **WHEN** neither backlight chip ACKs and the client posts color
- **THEN** the response is **503** with a backlight not-found style message

### Requirement: Missing LCD while enabled
When `lcd1602` is true but the LCD does not ACK at `0x3E`, write/clear/brightness requests that need hardware MUST answer **503** with a not-found style message. `GET` text/brightness MAY return the last known logical state from RAM/NVS. The enabled setting MUST remain true.

#### Scenario: Write without hardware
- **WHEN** `lcd1602` is true, no device ACKs at `0x3E`, and the client posts text
- **THEN** the response is **503** and `GET /settings` still reports `lcd1602` true

### Requirement: Last state persistence
After every successful text update, clear, brightness, or color change, the firmware MUST store last state: both lines’ text and align, brightness, and base RGB color. Factory reset MUST clear last state to empty lines, align left, default brightness, and white color (255,255,255).

#### Scenario: Boot last after power cycle
- **WHEN** last state was saved, `lcd1602_boot` is `last`, and the device reboots with the extension on and LCD present
- **THEN** the display restores that last text, align, brightness, and color

### Requirement: No outbound events
The LCD 1602 extension MUST NOT publish outbound events on WebSocket or serial.

#### Scenario: Text update silent on event bus
- **WHEN** a successful `POST /extensions/lcd1602/text` completes
- **THEN** no new outbound event is emitted for that update

### Requirement: LCD APIs available over serial ext
The LCD 1602 text, brightness, and color behaviors defined for HTTP `/extensions/lcd1602/…` MUST also apply when the same operations are invoked through serial `ext` with `id` `lcd1602` and the matching `suffix`/`method`. Disabled-extension, validation, missing-LCD, missing-backlight, and rgb-not-supported outcomes MUST match the HTTP API (including distinct not-found style messages for LCD vs backlight where HTTP already distinguishes them).

#### Scenario: Serial clear matches HTTP delete
- **WHEN** the host clears both lines via serial `ext` DELETE `text` while the extension is on
- **THEN** both lines are empty, brightness is unchanged, and last state is persisted as for `DELETE /extensions/lcd1602/text`

#### Scenario: Serial backlight missing
- **WHEN** the extension is on, the LCD ACKs, the backlight chip does not, and the host sets brightness via serial `ext`
- **THEN** the response is **503** with a backlight not-found style message
