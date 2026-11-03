# cli-host-daemon Specification

## Purpose

Keeps one local host daemon that owns the USB serial port and serves short-lived CLI commands, including npm installs and a repo checkout of the CLI.

## Requirements

### Requirement: Daemon stays up until stop
The host daemon MUST remain running after the command that started it exits. It MUST exit only for `daemon disable`, a replace, or a crash. It MUST NOT exit for `daemon stop`, a yield, a release, or because it has no clients.

#### Scenario: Serial command returns
- **WHEN** a serial command auto-starts the daemon and then exits successfully
- **THEN** the daemon is still running and still holds the serial port

#### Scenario: Explicit stop releases the port
- **WHEN** the user runs `daemon stop` while the daemon is running and no yield is requested
- **THEN** the daemon stays running, releases the serial port, writes no yield and no disable, and the next serial command may open the port

### Requirement: Start and stop are idempotent
`daemon start` MUST succeed when a daemon is already running, and `daemon stop` MUST succeed when no daemon is running. Neither command MUST start a second daemon. `daemon start` MUST clear a disable and a yield. `daemon stop` without yield MUST NOT record a disable.

#### Scenario: Start while running
- **WHEN** the user runs `daemon start` and a daemon is already running
- **THEN** the command exits successfully and the existing daemon keeps the serial port

#### Scenario: Stop while absent
- **WHEN** the user runs `daemon stop` and no daemon is running
- **THEN** the command exits successfully and does not record a disable

### Requirement: Serial commands auto-start the daemon
A robot command whose resolved transport is `serial` MUST auto-start the daemon when it is not running and neither a disable nor a yield is active, then use it. A command whose resolved transport is `http` MUST NOT start the daemon and MUST NOT send that command through it.

#### Scenario: First serial command starts the daemon
- **WHEN** no daemon is running, no disable is set, and the user runs `anim` with transport `serial`
- **THEN** the CLI starts the daemon, performs the animation through it, and the command may take longer than a later call against an already running daemon

#### Scenario: HTTP command leaves the daemon down
- **WHEN** no daemon is running and the user runs `anim` with transport `http`
- **THEN** the CLI uses HTTP and does not start a daemon

#### Scenario: Disable refuses auto-start
- **WHEN** a disable is set and the user runs `anim` with transport `serial`
- **THEN** the command fails, names the disable, and does not start a daemon

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
If the pid record is dead, or the pid is live but is not this daemon, the next serial command MUST NOT signal that pid. It MUST clear the stale record and start a daemon. If the pid is this daemon but the IPC endpoint does not complete a hello, the CLI MUST terminate that process and start a new daemon. When a login supervisor is installed, that start MUST go through the supervisor. The CLI MUST NOT spawn its own daemon process beside the supervisor.

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
A stop that requests yield MUST release the serial port and MUST refuse serial open for 180 seconds. The host daemon process MUST keep running. During that window a serial robot command MUST fail with an error that states the yield, and MUST NOT start a daemon. `daemon start` MUST clear the yield and MUST start the daemon when it is not running. `daemon start` MUST NOT open the serial port by itself. `daemon stop` without a yield request MUST NOT create a yield.

#### Scenario: Serial command during yield
- **WHEN** a yield is active and the user runs a serial `anim`
- **THEN** the command fails, no daemon starts, the serial port stays free, and a daemon that was already running is still running

#### Scenario: Manual start clears yield
- **WHEN** a yield is active and the user runs `daemon start`
- **THEN** the yield is cleared and the daemon is running

#### Scenario: Plain stop does not yield
- **WHEN** the user runs `daemon stop` without requesting yield and then immediately runs a serial command
- **THEN** that command may open the port and MUST NOT fail because of a yield

#### Scenario: Yield expiry leaves the port closed
- **WHEN** a yield expires while serial `watch` is still connected
- **THEN** the daemon leaves the port closed and does not deliver event frames until a later command opens the port

### Requirement: Release closes the port and leaves the process
`daemon release` MUST close the serial port and MUST leave the host daemon process running. `daemon stop` without `--yield` MUST do the same. Neither MUST write a yield or a disable. The next serial command MUST be allowed to open the port. Neither command MUST open the port by itself.

#### Scenario: Release then anim
- **WHEN** the daemon holds the serial port and the user runs `daemon release`, then a serial `anim`
- **THEN** the same daemon process serves `anim` and opens the port for that command

#### Scenario: Stop without yield matches release
- **WHEN** the daemon holds the serial port and the user runs `daemon stop` without `--yield`, then a serial `anim`
- **THEN** the same daemon process serves `anim` and opens the port for that command

#### Scenario: Release during play
- **WHEN** a serial play is active and the user runs `daemon release`
- **THEN** the command fails, playback continues, and the daemon still holds the port

### Requirement: Disable is the sticky opt-out
`daemon disable` MUST record a disable with no expiry, release the serial port, tell subscribers the daemon is stopping, and exit. An installed supervisor MUST NOT start a daemon again until `daemon start`. `daemon start` and `daemon restart` MUST clear the disable. `daemon disable` MUST succeed and record the disable when no daemon is running.

#### Scenario: Disable then anim
- **WHEN** the user runs `daemon disable` and then a serial `anim`
- **THEN** the command fails, names the disable, and does not start a daemon

#### Scenario: Start clears disable
- **WHEN** a disable is set and the user runs `daemon start`
- **THEN** the disable is clear, the daemon is running, and the serial port stays closed until a later command asks for it

#### Scenario: Disable while absent
- **WHEN** no daemon is running and the user runs `daemon disable`
- **THEN** the command succeeds and a later serial command does not auto-start until `daemon start`

### Requirement: Restart clears disable and yield
`daemon restart` MUST clear a disable and a yield, then leave one daemon running. `--port` on restart MUST be the port that process prefers the next time it opens a port. Restart MUST NOT open the port by itself.

#### Scenario: Restart after disable
- **WHEN** a disable is set and the user runs `daemon restart`
- **THEN** the disable is clear and `daemon status` shows a running daemon

### Requirement: Login install is per user
`daemon install` MUST register a start at user login for the current user. It MUST NOT register a start at system boot. The registration MUST start the wrapper and MUST NOT restart the wrapper or the daemon after either exits. A later `daemon install` MUST refresh the recorded package target and node path and MUST NOT start a second daemon. `daemon uninstall` MUST remove that registration and MUST NOT record a disable. Uninstall MUST succeed when nothing is installed.

#### Scenario: Install does not use system boot
- **WHEN** the user runs `daemon install`
- **THEN** the registration runs at that user's login and does not run as a system-boot service

#### Scenario: Second install refreshes the target
- **WHEN** a login registration exists and the user runs `daemon install` again from a different CLI path
- **THEN** the recorded package target matches that CLI and only one daemon is running

#### Scenario: Uninstall removes login start
- **WHEN** a login registration exists and the user runs `daemon uninstall`
- **THEN** the registration is gone, no disable is recorded, and login no longer starts the daemon

### Requirement: Installed supervisor is the only spawner
When a login supervisor is installed, every start of the daemon, including auto-start and replace, MUST be done by that supervisor through a stable wrapper. The wrapper MUST run the package target recorded for this user. The CLI MUST NOT spawn a daemon process itself. A replace MUST update that target and exit the current daemon. The wrapper MUST stay running and start a daemon from the new target. When no supervisor is installed, the CLI MUST spawn the replacement itself.

#### Scenario: Replace uses the supervisor
- **WHEN** a login supervisor is installed and a newer client replaces the daemon
- **THEN** the next hello comes from a process the wrapper started from the new package target, the wrapper did not exit, and the CLI did not spawn a second daemon

#### Scenario: No supervisor still spawns
- **WHEN** no login supervisor is installed and a serial command auto-starts the daemon
- **THEN** the CLI spawns a detached daemon and a closed terminal does not stop it

### Requirement: Install hands the process to the supervisor
`daemon install` while a daemon is running MUST leave exactly one daemon, and that process MUST be one the supervisor started. If a serial play is active, install MUST fail without stopping playback.

#### Scenario: Install during play
- **WHEN** a serial play is active and the user runs `daemon install`
- **THEN** the command fails, playback continues, and the existing daemon keeps the port

#### Scenario: Install replaces an orphan
- **WHEN** a daemon the CLI spawned is running and the user runs `daemon install`
- **THEN** one daemon remains and a later crash of that process is started again by the supervisor

### Requirement: Supervisor restarts a live daemon only
When a login supervisor is installed and a daemon that had accepted connections exits unexpectedly, the wrapper MUST start the daemon again. The OS registration MUST NOT do that restart. `daemon disable` MUST keep the wrapper from starting a daemon until `daemon start`. A process that exits before it accepts connections MUST NOT be restarted without bound.

#### Scenario: Crash after hello
- **WHEN** a login supervisor is installed and the daemon exits unexpectedly after it has accepted connections
- **THEN** the wrapper starts the daemon again

#### Scenario: Disable stays down
- **WHEN** a login supervisor is installed and the user runs `daemon disable`
- **THEN** the wrapper does not start a daemon until `daemon start`

#### Scenario: Exit before hello
- **WHEN** a login supervisor is installed and the daemon process exits before it accepts connections
- **THEN** the wrapper does not keep starting new processes in a loop
