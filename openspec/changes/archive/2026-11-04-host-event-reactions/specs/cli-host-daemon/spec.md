# Spec Delta

## MODIFIED Requirements

### Requirement: Start and stop are idempotent
`daemon start` MUST succeed when a daemon is already running, and `daemon stop` MUST succeed when no daemon is running. Neither command MUST start a second daemon. `daemon start` MUST clear a disable and a yield. `daemon stop` without yield MUST NOT record a disable. When the user-config transport is `serial` and at least one reaction rule is active, `daemon start` MUST open the serial port. Otherwise `daemon start` MUST NOT open a closed serial port.

#### Scenario: Start while running
- **WHEN** the user runs `daemon start` and a daemon is already running and holds the serial port
- **THEN** the command exits successfully and the existing daemon keeps the serial port

#### Scenario: Start opens an armed serial port
- **WHEN** the user runs `daemon start`, the serial port is closed, the user-config transport is `serial`, and at least one reaction rule is active
- **THEN** the command exits successfully and the daemon opens the serial port

#### Scenario: Start leaves an unarmed port closed
- **WHEN** no reaction rule is active and the user runs `daemon start`
- **THEN** the command exits successfully and a closed serial port stays closed

#### Scenario: Stop while absent
- **WHEN** the user runs `daemon stop` and no daemon is running
- **THEN** the command exits successfully and does not record a disable

### Requirement: Daemon status
`daemon status` MUST report whether the daemon is running. When it is running, the report MUST include pid, package version, channel (`npm` or `dev`), package root, protocol id, the open serial port or that the serial link is down, the reaction link state, the loaded rule count and whether the rules file is in error, and the last reaction result or that no reaction has run.

#### Scenario: Status while running
- **WHEN** the daemon is running and the user runs `daemon status`
- **THEN** the command exits successfully and prints pid, version, channel, package root, serial port state, reaction link state, rule count, and last reaction result

#### Scenario: Status while stopped
- **WHEN** no daemon is running and the user runs `daemon status`
- **THEN** the command exits successfully and reports that the daemon is not running

### Requirement: Upload yield blocks auto-start
A stop that requests yield MUST release the serial port and MUST refuse serial open for 180 seconds. The host daemon process MUST keep running. During that window a serial robot command MUST fail with an error that states the yield, and MUST NOT start a daemon. `daemon start` MUST clear the yield and MUST start the daemon when it is not running. `daemon start` MUST NOT open the serial port unless a serial reaction rule is active. `daemon stop` without a yield request MUST NOT create a yield.

#### Scenario: Serial command during yield
- **WHEN** a yield is active and the user runs a serial `anim`
- **THEN** the command fails, no daemon starts, the serial port stays free, and a daemon that was already running is still running

#### Scenario: Manual start clears yield
- **WHEN** a yield is active and the user runs `daemon start`
- **THEN** the yield is cleared and the daemon is running

#### Scenario: Manual start leaves the port closed when reactions are unarmed
- **WHEN** a yield is active, no reaction rule is active, and the user runs `daemon start`
- **THEN** the yield is cleared, the daemon is running, and the serial port stays closed

#### Scenario: Plain stop does not yield
- **WHEN** the user runs `daemon stop` without requesting yield and then immediately runs a serial command
- **THEN** that command may open the port and MUST NOT fail because of a yield

#### Scenario: Yield expiry leaves the port closed
- **WHEN** a yield expires while serial `watch` is still connected
- **THEN** the daemon leaves the port closed and does not deliver event frames until a later command opens the port

### Requirement: Disable is the sticky opt-out
`daemon disable` MUST record a disable with no expiry, release the serial port, tell subscribers the daemon is stopping, and exit. An installed supervisor MUST NOT start a daemon again until `daemon start`. `daemon start` and `daemon restart` MUST clear the disable. `daemon disable` MUST succeed and record the disable when no daemon is running.

#### Scenario: Disable then anim
- **WHEN** the user runs `daemon disable` and then a serial `anim`
- **THEN** the command fails, names the disable, and does not start a daemon

#### Scenario: Start clears disable
- **WHEN** a disable is set, no reaction rule is active, and the user runs `daemon start`
- **THEN** the disable is clear, the daemon is running, and the serial port stays closed until a later command asks for it

#### Scenario: Start after disable opens an armed serial port
- **WHEN** a disable is set, the user-config transport is `serial`, at least one reaction rule is active, and the user runs `daemon start`
- **THEN** the disable is clear, the daemon is running, and the serial port is open

#### Scenario: Disable while absent
- **WHEN** no daemon is running and the user runs `daemon disable`
- **THEN** the command succeeds and a later serial command does not auto-start until `daemon start`

### Requirement: Restart clears disable and yield
`daemon restart` MUST clear a disable and a yield, then leave one daemon running. `--port` on restart MUST be the port that process prefers the next time it opens a port. Restart MUST NOT open the port by itself unless the user-config transport is `serial` and at least one reaction rule is active, in which case restart MUST open the port.

#### Scenario: Restart after disable
- **WHEN** a disable is set, no reaction rule is active, and the user runs `daemon restart`
- **THEN** the disable is clear and `daemon status` shows a running daemon with the serial port closed

#### Scenario: Restart opens an armed serial port
- **WHEN** the user-config transport is `serial`, at least one reaction rule is active, and the user runs `daemon restart`
- **THEN** one daemon is running and the serial port is open
