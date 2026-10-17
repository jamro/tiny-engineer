# Spec Delta

## Purpose

Defines how the Tiny Engineer CLI selects HTTP or serial transport, discovers serial ports, and provides day-to-day robot control parity with the web UI when HTTP is unavailable.

## ADDED Requirements

### Requirement: Explicit transport selection
The CLI MUST resolve transport in this order: command-line flag, then process/project environment (`TINY_ENGINEER_TRANSPORT`), then the documented default `http`. The CLI MUST NOT choose between HTTP and serial by probing reachability or preferring a plugged USB device. A transport value written to the project `.env` by setup counts as an environment selection for later commands.

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
- **WHEN** `.env` sets `TINY_ENGINEER_TRANSPORT=serial` and the user passes `--transport http`
- **THEN** the CLI uses HTTP for that invocation

### Requirement: Project setup chooses host transport
The project `tiny-engineer setup` wizard MUST let the user choose host transport `http` or `serial` via an interactive prompt, or via non-interactive flags/env with the same meaning.

#### Scenario: Interactive serial setup
- **WHEN** the user runs interactive setup and chooses serial transport
- **THEN** setup records serial as the chosen host transport for that project

### Requirement: Project setup merges transport into .env
Setup MUST merge `TINY_ENGINEER_TRANSPORT` into the project `.env` using create/update/skip/force behavior consistent with `TINY_ENGINEER_URL` merge.

#### Scenario: Serial transport written to env
- **WHEN** setup completes with serial transport selected
- **THEN** the project `.env` contains `TINY_ENGINEER_TRANSPORT=serial`

#### Scenario: Commands use setup-written transport
- **WHEN** setup has written `TINY_ENGINEER_TRANSPORT=serial` and the user runs `anim` without a transport flag
- **THEN** the CLI uses the serial transport

### Requirement: Project setup HTTP still collects URL
When the chosen host transport is `http`, setup MUST collect a robot base URL and merge `TINY_ENGINEER_URL` as it does today, and MUST also persist transport as `http` in `.env`.

#### Scenario: Interactive HTTP setup writes URL and transport
- **WHEN** the user runs interactive setup, chooses HTTP, and provides a base URL
- **THEN** the project `.env` contains `TINY_ENGINEER_TRANSPORT=http` and `TINY_ENGINEER_URL` for that URL

### Requirement: Project setup serial does not require URL
When the chosen host transport is `serial`, setup MUST NOT require a robot URL. Setup MAY offer an optional serial port and MUST merge `TINY_ENGINEER_PORT` when the user provides one.

#### Scenario: Serial setup without URL
- **WHEN** the user completes serial transport setup without entering a URL
- **THEN** setup succeeds and does not fail for a missing `TINY_ENGINEER_URL`

### Requirement: Uninstall purges transport env keys
Uninstall purge of Tiny Engineer env keys MUST remove `TINY_ENGINEER_TRANSPORT` and `TINY_ENGINEER_PORT` along with URL and token keys.

#### Scenario: Purge removes transport
- **WHEN** the user runs uninstall with env purge after setup wrote transport keys
- **THEN** those `TINY_ENGINEER_TRANSPORT` and `TINY_ENGINEER_PORT` lines are removed from `.env`

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
Over the selected transport, the CLI MUST provide commands equivalent to the day-to-day web UI: animations, WAV play, settings get/set, hardware tests, servo test positioning, health, and factory reset (with confirmation). Setup-only calibration that remains AP-only in firmware MAY stay out of the CLI.

#### Scenario: Settings via CLI
- **WHEN** the user runs a settings get command with a valid transport
- **THEN** the CLI prints settings returned by the robot

#### Scenario: Hardware test via CLI
- **WHEN** the user runs a supported hardware test command
- **THEN** the CLI invokes the corresponding robot test operation

### Requirement: Serial monitor for LOG frames
The CLI MUST provide a monitor command that opens the serial port, decodes COBS frames, and displays the text of typed log frames. Non-log frames MUST NOT be printed as raw binary garbage; they MAY be summarized or ignored according to monitor options.

#### Scenario: Display log frames
- **WHEN** the device emits a log frame while monitor is running
- **THEN** the monitor prints the log text in human-readable form

### Requirement: Hooks and doctor respect transport
Hook posting, doctor health checks, and other robot-facing CLI entry points MUST use the same explicit transport configuration as `anim` / `play`.

#### Scenario: Doctor over serial
- **WHEN** transport is `serial` and the user runs doctor
- **THEN** doctor probes health over serial and does not require HTTP reachability

#### Scenario: Hook over serial
- **WHEN** transport is `serial` and a hook maps to an animation
- **THEN** the CLI posts that animation over serial with the existing fail-open timing behavior for offline robots

### Requirement: Serial play ends session on host failure
After a successful serial `play` open, if streaming fails before normal completion, the CLI MUST attempt `play_end` while the port is still open (best-effort), then close the port. This MUST NOT suppress the original stream failure error to the user.

#### Scenario: Credit or stream error still sends play_end
- **WHEN** serial play streaming fails (for example stream Error frame or credit wait timeout) after `play` has opened
- **THEN** the CLI attempts `play_end` for that stream before closing the port
