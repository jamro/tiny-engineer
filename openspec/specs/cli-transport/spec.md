# cli-transport Specification

## Purpose

Defines how the Tiny Engineer CLI selects HTTP or serial transport, discovers serial ports, and provides day-to-day robot control parity with the web UI when HTTP is unavailable.

## Requirements

### Requirement: Explicit transport selection
The CLI MUST resolve transport in this order: command-line flag, then environment sources for `TINY_ENGINEER_TRANSPORT` (process env, then project `.env`, then user config `env`, then other documented dotenv sources), then the documented default `http`. The CLI MUST NOT choose between HTTP and serial by probing reachability or preferring a plugged USB device. A transport value written to the user config `env` by setup counts as an environment selection for later commands when not overridden by a higher-precedence source.

#### Scenario: Serial transport selected
- **WHEN** the user runs a robot command with transport `serial`
- **THEN** the CLI uses the serial protocol and does not fall back to HTTP on failure

#### Scenario: HTTP transport selected
- **WHEN** the user runs a robot command with transport `http`
- **THEN** the CLI uses HTTP and does not fall back to serial on failure

#### Scenario: Unset transport defaults to HTTP
- **WHEN** neither a transport flag nor a transport environment variable is set
- **THEN** the CLI uses HTTP (backward compatible) and does not probe serial

#### Scenario: Flag overrides env
- **WHEN** an env source sets `TINY_ENGINEER_TRANSPORT=serial` and the user passes `--transport http`
- **THEN** the CLI uses HTTP for that invocation

#### Scenario: Legacy project env still overrides user config
- **WHEN** project `.env` sets `TINY_ENGINEER_TRANSPORT=serial` and user `env` sets `http`
- **THEN** the CLI uses serial when no transport flag is passed

### Requirement: Project setup chooses host transport
The `tiny-engineer setup` wizard MUST let the user choose host transport `http` or `serial` via an interactive prompt, or via non-interactive flags/env with the same meaning.

#### Scenario: Interactive serial setup
- **WHEN** the user runs interactive setup and chooses serial transport
- **THEN** setup records serial as the chosen host transport in user config

### Requirement: Project setup merges transport into .env
Setup MUST merge `TINY_ENGINEER_TRANSPORT` into the user config `env` file (not the project `.env`) using create / confirm-on-diff / keep-on-`--yes` behavior defined by the cli-user-config capability.

#### Scenario: Serial transport written to env
- **WHEN** setup completes with serial transport selected and no conflicting stored value blocks the write
- **THEN** the user config `env` contains `TINY_ENGINEER_TRANSPORT=serial`

#### Scenario: Commands use setup-written transport
- **WHEN** setup has written `TINY_ENGINEER_TRANSPORT=serial` to user config and the user runs `anim` without a transport flag and without a higher-precedence override
- **THEN** the CLI uses the serial transport

### Requirement: Project setup HTTP still collects URL
When the chosen host transport is `http`, setup MUST collect a robot base URL and merge `TINY_ENGINEER_URL` into the user config `env`, and MUST also persist transport as `http` there (subject to confirm-on-diff / `--yes` keep rules).

#### Scenario: Interactive HTTP setup writes URL and transport
- **WHEN** the user runs interactive setup, chooses HTTP, and provides a base URL with no conflicting stored values (or confirms replace)
- **THEN** the user config `env` contains `TINY_ENGINEER_TRANSPORT=http` and `TINY_ENGINEER_URL` for that URL

### Requirement: Project setup serial does not require URL
When the chosen host transport is `serial`, setup MUST NOT require a robot URL. Setup MAY offer an optional serial port and MUST merge `TINY_ENGINEER_PORT` into the user config `env` when the user provides one (subject to confirm-on-diff / `--yes` keep rules).

#### Scenario: Serial setup without URL
- **WHEN** the user completes serial transport setup without entering a URL
- **THEN** setup succeeds and does not fail for a missing `TINY_ENGINEER_URL`

### Requirement: Serial port override and autodetection
When transport is `serial`, the CLI MUST accept an explicit port path. If no port is provided, the CLI MUST attempt autodetection of a connected Tiny Engineer USB CDC device. Autodetection results that take more than a few milliseconds to obtain MUST be cached for later invocations, with cache invalidation when the cached port fails to open or fails a probe.

#### Scenario: Explicit port
- **WHEN** the user supplies a serial port path
- **THEN** the CLI opens that port without requiring autodetection

#### Scenario: Autodetect and cache
- **WHEN** no port is supplied and a Tiny Engineer device is found by probe
- **THEN** the CLI uses that port and stores it in the cache for subsequent runs

#### Scenario: Stale cache
- **WHEN** the cached port fails to open or fails probe
- **THEN** the CLI discards the cache entry and retries autodetection

### Requirement: Day-to-day command parity with web UI
Over the selected transport, the CLI MUST provide commands equivalent to the day-to-day web UI: animations, WAV play, settings get/set, hardware tests, servo test positioning, health, factory reset (with confirmation), and servo park. Setup-only calibration that remains AP-only in firmware MAY stay out of the CLI.

#### Scenario: Settings via CLI
- **WHEN** the user runs a settings get command with a valid transport
- **THEN** the CLI prints settings returned by the robot

#### Scenario: Hardware test via CLI
- **WHEN** the user runs a supported hardware test command
- **THEN** the CLI invokes the corresponding robot test operation

#### Scenario: Park via CLI
- **WHEN** the user runs the park command with a valid transport
- **THEN** the CLI invokes the robot park operation over that transport

### Requirement: Pre-upload may invoke CLI park over serial
PlatformIO pre-upload automation MUST be able to invoke the CLI park command with serial transport without requiring project `.env` transport to be serial. Failure of that park attempt MUST NOT abort the upload.

#### Scenario: Explicit serial transport for pre-upload
- **WHEN** the PlatformIO pre-upload hook runs park for flash safety
- **THEN** it requests serial transport explicitly rather than relying on the project's default transport

### Requirement: Serial transport uses the host daemon
When transport is `serial`, robot commands MUST perform serial I/O through the host daemon defined by `cli-host-daemon`. The short-lived command process MUST NOT open the serial device itself. HTTP transport MUST keep using HTTP and MUST NOT open a serial device.

#### Scenario: Anim shares the port with a running daemon
- **WHEN** the host daemon is running and the user runs `anim` with transport `serial`
- **THEN** the animation is sent through the daemon and the command does not fail because the serial port is busy

#### Scenario: HTTP anim does not open serial
- **WHEN** the user runs `anim` with transport `http`
- **THEN** the CLI uses HTTP and does not open a serial device

### Requirement: Serial monitor for LOG frames
The CLI MUST provide a monitor command that displays the text of typed serial log frames. The monitor process MUST NOT open the serial device; the host daemon owns it and the monitor receives log frames from that daemon. Non-log frames MUST NOT be printed as raw binary garbage; they MAY be summarized or ignored according to monitor options.

#### Scenario: Display log frames
- **WHEN** the device emits a log frame while monitor is running
- **THEN** the monitor prints the log text in human-readable form

#### Scenario: Monitor stays up during another serial command
- **WHEN** monitor is running and the user runs a serial `anim` command
- **THEN** the anim command completes and monitor is still running afterward

### Requirement: Hooks and doctor respect transport
Hook posting, doctor health checks, and other robot-facing CLI entry points MUST use the same explicit transport configuration as `anim` / `play`. When that transport is `serial`, they MUST use the host daemon, including auto-start, so a running serial `watch` or `monitor` does not make the call fail because the port is busy.

#### Scenario: Doctor over serial
- **WHEN** transport is `serial` and the user runs doctor
- **THEN** doctor probes health over serial and does not require HTTP reachability

#### Scenario: Hook over serial
- **WHEN** transport is `serial` and a hook maps to an animation
- **THEN** the CLI posts that animation over serial with the existing fail-open timing behavior for offline robots

#### Scenario: Hook while watch holds serial
- **WHEN** transport is `serial`, `watch` is running, and a hook maps to an animation
- **THEN** the hook posts that animation through the host daemon instead of failing because the port is open

### Requirement: Serial play ends session on host failure
After a successful serial `play` open, if streaming fails before normal completion, the CLI MUST attempt `play_end` while the serial port is still open (best-effort). Finishing the command MUST NOT release the serial device; the host daemon keeps it. This MUST NOT suppress the original stream failure error to the user.

#### Scenario: Credit or stream error still sends play_end
- **WHEN** serial play streaming fails (for example stream Error frame or credit wait timeout) after `play` has opened
- **THEN** the CLI attempts `play_end` for that stream and the host daemon still holds the serial port
