## ADDED Requirements

### Requirement: Daemon probes ordered serial candidates
The host daemon MUST accept a probe request with an ordered list of serial port paths. For each path in order until one syncs successfully, the daemon MUST attempt open-and-sync when no session is open, or sync-only when the session is already that path. When a session is already open on a different path, the daemon MUST NOT close that session to try other candidates; it MUST report those other paths as not probed because the held path is in use. A successful probe when no session was open MUST leave the serial session open on the winning path. Probe MUST NOT write the serial port cache. Probe MUST fail while a serial play is active. Probe MUST respect disable and yield the same as other serial opens (no open while those are active).

#### Scenario: Walk when the port is free
- **WHEN** the daemon has no serial session and receives a probe with two candidate paths where the first fails open or sync and the second syncs
- **THEN** the daemon reports failure for the first, success for the second, and keeps a session open on the second path without writing the port cache

#### Scenario: Held port is not yanked for other candidates
- **WHEN** the daemon holds path A and receives a probe listing A then B
- **THEN** the daemon syncs A and does not close A to open B

#### Scenario: Probe refused during play
- **WHEN** a serial play is active and a probe request arrives
- **THEN** the daemon refuses the probe and keeps the play session
