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
When enabled, the extension MUST use LCD address **0x3E** (AiP31068L). Controllable backlight MUST auto-detect **0x6B** (SN3193, Blue/Gray/Green) or **0x60** (PCA9633, RGB module); those addresses MUST NOT be settings. Brightness on PCA9633 MUST map 0–100 to equal white R/G/B PWM channels. The extension MUST target the Waveshare LCD1602 I2C module family, not PCF8574 backpacks.

#### Scenario: No address settings
- **WHEN** a client posts an unknown field intended as an LCD I2C address
- **THEN** `POST /settings` rejects it as an unknown or invalid field and does not save it

### Requirement: Backlight on boot setting
The setting `lcd1602_backlight_boot` MUST be a boolean on `GET`/`POST /settings`, default **true** when unset, including after factory reset. It MUST persist when `lcd1602` is false. When the extension enables or boots with the extension on: if true, the firmware MUST turn the backlight on (default brightness unless last/config supplies another); if false, the backlight MUST stay off until a successful brightness API call.

#### Scenario: Default true
- **WHEN** the device has no stored `lcd1602_backlight_boot`
- **THEN** `GET /settings` reports `lcd1602_backlight_boot` true

#### Scenario: Wait for API
- **WHEN** `lcd1602` is true, `lcd1602_backlight_boot` is false, and boot content does not require a brightness from last
- **THEN** the backlight remains off until `POST /extensions/lcd1602/brightness` succeeds

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
- **THEN** the section shows backlight-on-boot, boot-content mode, Waveshare I2C Extension-port wiring, and (when mode is `config`) the two line fields

#### Scenario: Config omits LCD
- **WHEN** the user opens the Config page
- **THEN** that page has no LCD 1602 controls

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
`GET /extensions/lcd1602/brightness` MUST return JSON `{"brightness": <0-100>}`. `POST /extensions/lcd1602/brightness` MUST accept `{"brightness": <0-100>}`, apply backlight (0 = off), persist last state, and answer **200** with the new value. Out-of-range or bad types MUST answer **400** without changing brightness.

#### Scenario: Set brightness
- **WHEN** the client posts `{"brightness": 50}` while the extension is on and LCD backlight is reachable
- **THEN** backlight is set near 50% and `GET` reports `50`

#### Scenario: Invalid brightness
- **WHEN** the client posts `{"brightness": 101}`
- **THEN** the response is **400** and brightness is unchanged

### Requirement: Missing LCD while enabled
When `lcd1602` is true but the LCD does not ACK at `0x3E`, write/clear/brightness requests that need hardware MUST answer **503** with a not-found style message. `GET` text/brightness MAY return the last known logical state from RAM/NVS. The enabled setting MUST remain true.

#### Scenario: Write without hardware
- **WHEN** `lcd1602` is true, no device ACKs at `0x3E`, and the client posts text
- **THEN** the response is **503** and `GET /settings` still reports `lcd1602` true

### Requirement: Last state persistence
After every successful text update, clear, or brightness change, the firmware MUST store last state: both lines’ text and align, plus brightness. Factory reset MUST clear last state to empty lines, align left, and a defined default brightness (implementation default, documented).

#### Scenario: Boot last after power cycle
- **WHEN** last state was saved, `lcd1602_boot` is `last`, and the device reboots with the extension on and LCD present
- **THEN** the display restores that last text, align, and brightness

### Requirement: No outbound events
The LCD 1602 extension MUST NOT publish outbound events on WebSocket or serial.

#### Scenario: Text update silent on event bus
- **WHEN** a successful `POST /extensions/lcd1602/text` completes
- **THEN** no new outbound event is emitted for that update

### Requirement: LCD APIs available over serial ext
The LCD 1602 text and brightness behaviors defined for HTTP `/extensions/lcd1602/…` MUST also apply when the same operations are invoked through serial `ext` with `id` `lcd1602` and the matching `suffix`/`method`. Disabled-extension, validation, missing-LCD, and missing-backlight outcomes MUST match the HTTP API (including distinct not-found style messages for LCD vs backlight where HTTP already distinguishes them).

#### Scenario: Serial clear matches HTTP delete
- **WHEN** the host clears both lines via serial `ext` DELETE `text` while the extension is on
- **THEN** both lines are empty, brightness is unchanged, and last state is persisted as for `DELETE /extensions/lcd1602/text`

#### Scenario: Serial backlight missing
- **WHEN** the extension is on, the LCD ACKs, the backlight chip does not, and the host sets brightness via serial `ext`
- **THEN** the response is **503** with a backlight not-found style message
