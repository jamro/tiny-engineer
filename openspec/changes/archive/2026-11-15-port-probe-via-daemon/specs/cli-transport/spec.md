## ADDED Requirements

### Requirement: Port probe uses the host daemon
`tiny-engineer port --probe` MUST perform sync attempts through the host daemon defined by `cli-host-daemon`. The short-lived `port` process MUST NOT open the serial device for probe. Listing and scoring host ports (`SerialPort.list` and score rules) MAY stay in the CLI process because they do not take exclusive ownership of the device. Probe MUST use the same candidate order as today (explicit env port alone when set; otherwise cached port then other positive-score autodetect paths). Probe MUST NOT write the serial port cache. Disable and yield MUST refuse probe the same way they refuse other serial robot entry points (error names the disable or yield; no daemon start while those are active). Auto-start MUST apply when neither disable nor yield is active and the daemon is not running.

#### Scenario: Probe while daemon holds the port
- **WHEN** the host daemon already has a serial session open and the user runs `port --probe`
- **THEN** the CLI does not open the tty itself, the daemon syncs that open path, and the command does not fail solely because the port is busy with the daemon

#### Scenario: Probe does not write the cache
- **WHEN** `port --probe` finds a candidate that syncs successfully
- **THEN** the serial port cache file is left unchanged by that probe

#### Scenario: Probe honors yield
- **WHEN** a yield is active and the user runs `port --probe`
- **THEN** the command fails, names the yield, and does not start a daemon or open the serial device in the CLI process
