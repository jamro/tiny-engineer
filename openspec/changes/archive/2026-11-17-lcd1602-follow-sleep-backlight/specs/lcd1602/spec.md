# Spec Delta

## REMOVED Requirements

### Requirement: Backlight on boot setting
**Reason**: Replaced by follow-sleep backlight; the old false case (off until brightness API) conflicted with sleep/wake dimming.
**Migration**: Use `lcd1602_backlight_sleep`. Existing NVS `ext_lcd_bl` values keep their bool; interpret true as follow sleep.

## ADDED Requirements

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
- **THEN** the section shows a **Follow sleep** backlight toggle bound to `lcd1602_backlight_sleep`, boot-content mode, Waveshare I2C Extension-port wiring, and (when mode is `config`) the two line fields

#### Scenario: Config omits LCD
- **WHEN** the user opens the Config page
- **THEN** that page has no LCD 1602 controls
