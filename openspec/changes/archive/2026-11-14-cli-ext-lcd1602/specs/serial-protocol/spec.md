# Spec Delta

## ADDED Requirements

### Requirement: Serial settings include LCD 1602 fields
`settings.get` over serial MUST include the `lcd1602`, `lcd1602_backlight_boot`, `lcd1602_boot`, `lcd1602_line1`, and `lcd1602_line2` fields with the same types and defaults as `GET /settings`. `settings.set` MUST accept those keys with the same validation and immediate-apply behavior as `POST /settings`. Other settings fields not already returned or accepted by serial MAY remain deferred (full GET `/settings` parity is not claimed by this change alone).

#### Scenario: Enable LCD over serial
- **WHEN** the host sends `settings.set` with `{"lcd1602": true}` after provisioning
- **THEN** the extension becomes active without reboot and a later `settings.get` reports `lcd1602` true

#### Scenario: Invalid boot mode rejected
- **WHEN** the host sends `settings.set` with `{"lcd1602_boot": "welcome"}`
- **THEN** the device answers **400** and does not save the value

### Requirement: Ext op is part of the serial control surface
The serial protocol MUST document and implement `op` **`ext`** for extension APIs as specified by the `extension-serial` capability. Day-to-day control over serial MUST include `ext` for registered extensions (LCD 1602 in this change).

#### Scenario: Ext listed among control ops
- **WHEN** a provisioned host sends a valid `ext` request for a registered LCD route
- **THEN** the device does not reject solely because `ext` is an unknown op

## MODIFIED Requirements

### Requirement: Feature parity with HTTP control surface
The serial API MUST expose the same day-to-day control operations as the HTTP JSON API, including animations, settings read/write, hardware tests, servo test, health, factory reset, streamed WAV play with optional animation name, servo park, and **extension APIs via `ext` for registered extensions**. Settings read/write MUST include at least the fields already supported plus the **`lcd1602*` slice** in this change; remaining HTTP settings fields remain a known gap until a later change closes full GET `/settings` parity.

#### Scenario: Settings over serial
- **WHEN** the host requests current settings over serial
- **THEN** the device returns at least the previously supported serial settings fields plus the `lcd1602*` fields (excluding secrets)

#### Scenario: Play with pose over serial
- **WHEN** the host streams a WAV with an allowed play animation name over serial
- **THEN** the device plays the clip while running that animation, matching `POST /play` behavior

#### Scenario: Park over serial matches HTTP
- **WHEN** the host sends serial op `servos_park`
- **THEN** the device applies the same park semantics as `POST /servos/park`

#### Scenario: Extension text over serial
- **WHEN** the host sends a valid `ext` POST for LCD text while the extension is on and hardware is present
- **THEN** the device updates the display with the same semantics as `POST /extensions/lcd1602/text`
