# cli-host-daemon Specification

## Purpose

Keeps one local host daemon that owns the USB serial port and serves short-lived CLI commands, including npm installs and a repo checkout of the CLI.

## Requirements

### Requirement: Daemon stays up until stop
The host daemon MUST remain running after the command that started it exits. It MUST exit only for an explicit stop, a replace, or a crash. It MUST NOT exit only because it has no clients.

#### Scenario: Serial command returns
- **WHEN** a serial command auto-starts the daemon and then exits successfully
- **THEN** the daemon is still running and still holds the serial port

#### Scenario: Explicit stop releases the port
- **WHEN** the user runs `daemon stop` while the daemon is running and no yield is requested
- **THEN** the daemon exits, releases the serial port, and the next serial command may auto-start it immediately

### Requirement: Start and stop are idempotent
`daemon start` MUST succeed when a daemon is already running, and `daemon stop` MUST succeed when no daemon is running. Neither command MUST start a second daemon.

#### Scenario: Start while running
- **WHEN** the user runs `daemon start` and a daemon is already running
- **THEN** the command exits successfully and the existing daemon keeps the serial port

#### Scenario: Stop while absent
- **WHEN** the user runs `daemon stop` and no daemon is running
- **THEN** the command exits successfully

### Requirement: Serial commands auto-start the daemon
A robot command whose resolved transport is `serial` MUST auto-start the daemon when it is not running, then use it. A command whose resolved transport is `http` MUST NOT start the daemon and MUST NOT send that command through it.

#### Scenario: First serial command starts the daemon
- **WHEN** no daemon is running and the user runs `anim` with transport `serial`
- **THEN** the CLI starts the daemon, performs the animation through it, and the command may take longer than a later call against an already running daemon

#### Scenario: HTTP command leaves the daemon down
- **WHEN** no daemon is running and the user runs `anim` with transport `http`
- **THEN** the CLI uses HTTP and does not start a daemon

### Requirement: Runtime files follow user config
Daemon IPC endpoint, pid record, and log MUST live in the same user config directory as the user `env` file, including when `TINY_ENGINEER_CONFIG_DIR` is set. On POSIX the IPC endpoint MUST be reachable only by the owning user.

#### Scenario: Isolated config directory
- **WHEN** `TINY_ENGINEER_CONFIG_DIR` points at a directory and a serial command starts the daemon
- **THEN** the daemon IPC endpoint is created inside that directory and not in the default user config directory

### Requirement: Daemon status
`daemon status` MUST report whether the daemon is running. When it is running, the report MUST include pid, package version, channel (`npm` or `dev`), package root, protocol id, and the open serial port or that the link is down.

#### Scenario: Status while running
- **WHEN** the daemon is running and the user runs `daemon status`
- **THEN** the command exits successfully and prints pid, version, channel, package root, and serial port state

#### Scenario: Status while stopped
- **WHEN** no daemon is running and the user runs `daemon status`
- **THEN** the command exits successfully and reports that the daemon is not running

### Requirement: npm clients replace only an older npm daemon
A client launched from an install whose path contains `node_modules/tiny-engineer` is channel `npm`. An npm client MUST replace the daemon only when the daemon is also channel `npm` and the client's package version is a higher SemVer. The replacement process MUST be started from that client's own executable and package entry, not from a git URL. The same SemVer across different `npx` cache paths MUST NOT replace the daemon.

#### Scenario: Published update replaces the daemon
- **WHEN** an npm client of a higher SemVer connects to an npm daemon of a lower SemVer
- **THEN** the CLI stops that daemon and starts a daemon from the newer package, then completes the original command

#### Scenario: Same published version in another cache path
- **WHEN** an npm client of the same SemVer and the same package contents connects from a different `npx` cache path
- **THEN** the existing daemon stays and serves the command

#### Scenario: Older npm client does not downgrade
- **WHEN** an npm client of a lower SemVer connects to an npm daemon of a higher SemVer and understands the daemon protocol
- **THEN** the existing daemon stays and serves the command

### Requirement: Dev checkout owns the daemon over npm
A client launched from `packages/tiny-engineer-cli` outside `node_modules` is channel `dev`. A dev client MUST replace an npm daemon even when the checkout's package version is lower or equal. An npm client MUST NOT replace a dev daemon. A dev client MUST replace a dev daemon of the same package root when the on-disk source identity changed since that daemon started. A dev client MUST NOT auto-replace a dev daemon whose package root is a different checkout.

#### Scenario: Local CLI replaces a published daemon
- **WHEN** a dev client runs while an npm daemon is running
- **THEN** the CLI replaces the daemon with one started from that checkout's CLI entry and then completes the command

#### Scenario: npm hook leaves a dev daemon in place
- **WHEN** an npm client runs while a dev daemon is running
- **THEN** the dev daemon stays and serves the command

#### Scenario: Edited checkout refreshes the daemon
- **WHEN** a dev daemon is running from a package root and a dev client from that same root sees a changed source identity
- **THEN** the CLI replaces the daemon from that root and then completes the command

#### Scenario: Second checkout does not steal the daemon
- **WHEN** a dev daemon is running from one checkout and a dev client from a different checkout connects
- **THEN** the existing daemon stays

### Requirement: Protocol mismatch does not kill a newer daemon
A client that does not understand the daemon's protocol MUST NOT stop the daemon. A client that speaks a newer protocol than the daemon MUST stop it and start a daemon from the client's own package.

#### Scenario: Older client sees an unknown protocol
- **WHEN** a client receives a hello it does not understand
- **THEN** the command fails with an error that tells the user to update the CLI, and the daemon is still running

#### Scenario: Newer client replaces an older protocol
- **WHEN** a client speaks a newer protocol than the running daemon
- **THEN** the CLI stops that daemon, starts one from the client's package, and completes the command if the new daemon is compatible

### Requirement: Replace does not cut an active play
The daemon MUST NOT abort an active serial play in order to replace itself. A replace requested during play MUST wait until that play ends, or the requesting command MUST fail without stopping playback. A later command after play ends MUST still apply a pending replace rule.

#### Scenario: Compatible command during play
- **WHEN** a client that would replace the daemon connects during an active serial play and still understands the running protocol
- **THEN** playback continues and the daemon is not stopped before that play finishes

### Requirement: Auto-start does not loop
One command MUST attempt to start or replace the daemon at most once after a failed hello. A second failure in that command MUST exit non-zero and MUST NOT spawn again.

#### Scenario: Daemon crashes on startup
- **WHEN** a serial command starts the daemon and the new process exits before a successful hello
- **THEN** the command fails and does not leave a restart loop

### Requirement: Foreign and wedged processes
If the pid record is dead, or the pid is live but is not this daemon, the next serial command MUST NOT signal that pid. It MUST clear the stale record and start a daemon. If the pid is this daemon but the IPC endpoint does not complete a hello, the CLI MUST terminate that process and start a new daemon.

#### Scenario: Reused pid
- **WHEN** the pid record names a live process that is not the host daemon
- **THEN** the CLI does not signal that process, removes the stale record, and starts a daemon

#### Scenario: Wedged daemon
- **WHEN** the pid record names a live host daemon and the IPC endpoint does not produce a hello
- **THEN** the CLI terminates that daemon and starts a new one

### Requirement: One daemon wins concurrent starts
When two serial commands auto-start at the same time, exactly one daemon MUST own the IPC endpoint. The other command MUST use that daemon.

#### Scenario: Two anims start together
- **WHEN** two serial commands start while no daemon is running
- **THEN** both commands complete through a single daemon process

### Requirement: Requested port must match the open port
If the daemon already holds a serial port and a command asks for a different port path, the command MUST fail and name both paths. It MUST NOT switch ports while the current link is open.

#### Scenario: Second port rejected
- **WHEN** the daemon holds port A and the user runs a serial command with `--port` B
- **THEN** the command fails, the error names A and B, and the daemon still holds A

### Requirement: Starting terminal can close
The daemon MUST keep running if the terminal or parent process that auto-started it exits.

#### Scenario: Parent shell exits
- **WHEN** a serial command auto-starts the daemon and that shell then exits
- **THEN** `daemon status` from a new shell still shows the daemon running

### Requirement: Upload yield blocks auto-start
A stop that requests yield MUST release the serial port and MUST refuse auto-start for 180 seconds. During that window a serial robot command MUST fail with an error that states the yield, and MUST NOT start the daemon. `daemon start` MUST clear the yield and start the daemon. `daemon stop` without a yield request MUST NOT create a yield.

#### Scenario: Serial command during yield
- **WHEN** a yield is active and the user runs a serial `anim`
- **THEN** the command fails, no daemon starts, and the serial port stays free

#### Scenario: Manual start clears yield
- **WHEN** a yield is active and the user runs `daemon start`
- **THEN** the yield is cleared and the daemon is running

#### Scenario: Plain stop does not yield
- **WHEN** the user runs `daemon stop` without requesting yield and then immediately runs a serial command
- **THEN** that command may auto-start the daemon
