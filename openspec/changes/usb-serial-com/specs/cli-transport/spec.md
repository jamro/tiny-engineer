# Spec Delta

## Purpose

Defines how the Tiny Engineer CLI selects HTTP or serial transport, discovers serial ports, and provides day-to-day robot control parity with the web UI when HTTP is unavailable.

## ADDED Requirements

### Requirement: Explicit transport selection
The CLI MUST select transport only from an explicit flag, environment variable, or the documented default `http`. The CLI MUST NOT choose between HTTP and serial by probing reachability or preferring a plugged USB device.

#### Scenario: Serial transport selected
- **WHEN** the user runs a robot command with transport `serial`
- **THEN** the CLI uses the serial protocol and does not fall back to HTTP on failure

#### Scenario: HTTP transport selected
- **WHEN** the user runs a robot command with transport `http`
- **THEN** the CLI uses HTTP and does not fall back to serial on failure

#### Scenario: Unset transport defaults to HTTP
- **WHEN** neither a transport flag nor a transport environment variable is set
- **THEN** the CLI uses HTTP (backward compatible) and does not probe serial

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
