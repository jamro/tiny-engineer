# Spec Delta

## MODIFIED Requirements

### Requirement: CLI watch exits when the link cannot open
`watch` MUST exit non-zero and print an error when the selected link cannot be opened. After open it MUST keep running until the user interrupts it, the remote link ends, or the user runs `daemon disable`. A replace MUST NOT end `watch`; `watch` MUST continue on the new daemon. `daemon disable` MUST end `watch` without starting a daemon. A crash with no login supervisor MUST make `watch` retry once. A crash that a login supervisor recovers MUST NOT end `watch`.

#### Scenario: Unreachable robot
- **WHEN** the user runs `watch` and the selected link cannot be opened
- **THEN** the command exits non-zero and prints an error

#### Scenario: User stops the daemon
- **WHEN** serial `watch` is running and the user runs `daemon stop`
- **THEN** `watch` stays running and does not exit

#### Scenario: User disables the daemon
- **WHEN** serial `watch` is running and the user runs `daemon disable`
- **THEN** `watch` exits and does not auto-start a daemon

#### Scenario: Daemon is replaced
- **WHEN** serial `watch` is running and a client replaces the daemon
- **THEN** `watch` continues and prints event frames from the replacement daemon

#### Scenario: Daemon crashes
- **WHEN** serial `watch` is running, no login supervisor is installed, and the daemon process crashes
- **THEN** `watch` attempts one reconnect and exits non-zero if the daemon does not come back

#### Scenario: Supervisor brings the daemon back
- **WHEN** serial `watch` is running, a login supervisor is installed, and the daemon crashes after it had accepted connections
- **THEN** `watch` stays running and prints later event frames from the restarted daemon

## ADDED Requirements

### Requirement: Release and yield leave watch running
`daemon release`, `daemon stop` without `--yield`, and `daemon stop --yield` MUST NOT end a running serial `watch`. `watch` MUST NOT start a daemon because of those commands. After a yield expires, `watch` MUST stay running and MUST NOT print event frames until a later command opens the port.

#### Scenario: Release leaves watch running
- **WHEN** serial `watch` is running and the user runs `daemon release`
- **THEN** `watch` stays running and does not exit

#### Scenario: Stop without yield leaves watch running
- **WHEN** serial `watch` is running and the user runs `daemon stop` without `--yield`
- **THEN** `watch` stays running and does not exit

#### Scenario: Yield leaves watch running
- **WHEN** serial `watch` is running and the user runs `daemon stop --yield`
- **THEN** `watch` stays running, and after the yield expires it still prints nothing until a later command opens the port

### Requirement: Uninstall ends watch
`daemon uninstall` of a supervisor-owned daemon MUST end a running serial `watch` the same way `daemon disable` does. Uninstall MUST NOT record a disable. The next serial command MAY auto-start a daemon.

#### Scenario: Uninstall while watch is running
- **WHEN** serial `watch` is running on a supervisor-owned daemon and the user runs `daemon uninstall`
- **THEN** `watch` exits, no disable is recorded, and a later serial command may auto-start a daemon
