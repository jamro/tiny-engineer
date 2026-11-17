# Spec Delta

## MODIFIED Requirements

### Requirement: Serial settings include LCD 1602 fields
`settings.get` over serial MUST include the `lcd1602`, `lcd1602_backlight_sleep`, `lcd1602_welcome_hello`, `lcd1602_boot`, `lcd1602_line1`, and `lcd1602_line2` fields with the same types and defaults as `GET /settings`. `settings.set` MUST accept those keys with the same validation and immediate-apply behavior as `POST /settings`. Other settings fields not already returned or accepted by serial MAY remain deferred (full GET `/settings` parity is not claimed by this change alone).

#### Scenario: Enable LCD over serial
- **WHEN** the host sends `settings.set` with `{"lcd1602": true}` after provisioning
- **THEN** the extension becomes active without reboot and a later `settings.get` reports `lcd1602` true

#### Scenario: Invalid boot mode rejected
- **WHEN** the host sends `settings.set` with `{"lcd1602_boot": "welcome"}`
- **THEN** the device answers **400** and does not save the value

#### Scenario: Follow sleep field over serial
- **WHEN** the host sends `settings.set` with `{"lcd1602_backlight_sleep": false}` after provisioning
- **THEN** a later `settings.get` reports `lcd1602_backlight_sleep` false and does not include `lcd1602_backlight_boot`

#### Scenario: Welcome hello field over serial
- **WHEN** the host sends `settings.set` with `{"lcd1602_welcome_hello": true}` after provisioning
- **THEN** a later `settings.get` reports `lcd1602_welcome_hello` true
