# Spec Delta

## MODIFIED Requirements

### Requirement: CLI watch over serial reads event frames
On transport `serial`, `watch` MUST receive event frames from the host daemon the same way other serial commands use that daemon, and MUST NOT open the serial device itself. It MUST NOT require a robot URL. It MUST print each `Evt` JSON payload as one line and MUST NOT print non-event frames as raw binary.

#### Scenario: Serial watch prints event frames
- **WHEN** transport is `serial` and the device sends an `Evt` frame
- **THEN** the command prints that frame's JSON as a single line and does not open a WebSocket

#### Scenario: Serial watch ignores other frames
- **WHEN** transport is `serial` and the device sends a log or response frame
- **THEN** the command does not print that frame as raw binary

#### Scenario: Serial watch needs no URL
- **WHEN** transport is `serial` and no robot HTTP URL is configured
- **THEN** `watch` still runs through the host daemon and does not fail for a missing URL

#### Scenario: Serial watch survives another serial command
- **WHEN** transport is `serial`, `watch` is running, and another process runs a serial `anim`
- **THEN** `watch` stays running and keeps printing later event frames

### Requirement: CLI watch exits when the link cannot open
`watch` MUST exit non-zero and print an error when the selected link cannot be opened. After open it MUST keep running until the user interrupts it, the remote link ends, or the daemon stops on purpose. A replace MUST NOT end `watch`; `watch` MUST continue on the new daemon. A user stop MUST end `watch` without starting a daemon. After a daemon crash, `watch` MUST retry once and MUST exit non-zero if that retry fails.

#### Scenario: Unreachable robot
- **WHEN** the user runs `watch` and the selected link cannot be opened
- **THEN** the command exits non-zero and prints an error

#### Scenario: User stops the daemon
- **WHEN** serial `watch` is running and the user runs `daemon stop`
- **THEN** `watch` exits and does not auto-start a daemon

#### Scenario: Daemon is replaced
- **WHEN** serial `watch` is running and a client replaces the daemon
- **THEN** `watch` continues and prints event frames from the replacement daemon

#### Scenario: Daemon crashes
- **WHEN** serial `watch` is running and the daemon process crashes
- **THEN** `watch` attempts one reconnect and exits non-zero if the daemon does not come back
