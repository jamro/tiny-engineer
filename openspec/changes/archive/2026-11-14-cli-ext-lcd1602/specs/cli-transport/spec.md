# Spec Delta

## ADDED Requirements

### Requirement: CLI ext command for registered extensions
The CLI MUST provide an `ext <id> …` command family that targets extension APIs over the selected transport (`http` or `serial`). For id `lcd1602`, the CLI MUST support text get, text set (partial lines/align), text clear, brightness get, and brightness set. Unknown `<id>` values that the CLI does not know how to map MUST fail locally with a clear error without contacting the robot.

#### Scenario: LCD text over HTTP
- **WHEN** the user runs `tiny-engineer ext lcd1602 text set` with line arguments and transport `http`
- **THEN** the CLI issues `POST /extensions/lcd1602/text` and prints or confirms the resulting state

#### Scenario: LCD brightness over serial
- **WHEN** the user runs `tiny-engineer ext lcd1602 brightness set 20` with transport `serial`
- **THEN** the CLI sends serial `ext` for brightness and does not fall back to HTTP

#### Scenario: Unknown extension id
- **WHEN** the user runs `tiny-engineer ext nope text get`
- **THEN** the CLI exits non-zero with an unknown-extension style error and does not open a robot session solely for that id

### Requirement: Ext uses the same transport resolution as other robot commands
`ext` MUST honor the existing transport resolution order (flag → env/config → default `http`) and the same URL/port/token rules as other day-to-day robot commands.

#### Scenario: Serial transport flag
- **WHEN** the user passes `--transport serial` to an `ext lcd1602` command
- **THEN** the CLI uses serial and does not fall back to HTTP on failure
