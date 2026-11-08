# Spec Delta

## ADDED Requirements

### Requirement: Serial transport uses the host daemon
When transport is `serial`, robot commands MUST perform serial I/O through the host daemon defined by `cli-host-daemon`. The short-lived command process MUST NOT open the serial device itself. HTTP transport MUST keep using HTTP and MUST NOT open a serial device.

#### Scenario: Anim shares the port with a running daemon
- **WHEN** the host daemon is running and the user runs `anim` with transport `serial`
- **THEN** the animation is sent through the daemon and the command does not fail because the serial port is busy

#### Scenario: HTTP anim does not open serial
- **WHEN** the user runs `anim` with transport `http`
- **THEN** the CLI uses HTTP and does not open a serial device

## MODIFIED Requirements

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
