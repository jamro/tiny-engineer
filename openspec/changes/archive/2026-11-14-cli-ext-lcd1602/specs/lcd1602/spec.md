# Spec Delta

## ADDED Requirements

### Requirement: LCD APIs available over serial ext
The LCD 1602 text and brightness behaviors defined for HTTP `/extensions/lcd1602/…` MUST also apply when the same operations are invoked through serial `ext` with `id` `lcd1602` and the matching `suffix`/`method`. Disabled-extension, validation, missing-LCD, and missing-backlight outcomes MUST match the HTTP API (including distinct not-found style messages for LCD vs backlight where HTTP already distinguishes them).

#### Scenario: Serial clear matches HTTP delete
- **WHEN** the host clears both lines via serial `ext` DELETE `text` while the extension is on
- **THEN** both lines are empty, brightness is unchanged, and last state is persisted as for `DELETE /extensions/lcd1602/text`

#### Scenario: Serial backlight missing
- **WHEN** the extension is on, the LCD ACKs, the backlight chip does not, and the host sets brightness via serial `ext`
- **THEN** the response is **503** with a backlight not-found style message
