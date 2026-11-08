# Spec Delta

## ADDED Requirements

### Requirement: Replace aborts an active play
When the CLI decides a running daemon must be replaced, it MUST stop that daemon even if a serial play is active. Stopping for replace MUST end the in-flight play (serial link closed; the play caller fails). After the old process exits, the CLI MUST start a daemon from the replacing client's package and MUST complete the original command when the new daemon is compatible. Plain `daemon release` without force or yield MUST still refuse while play is active.

#### Scenario: Replace during play upgrades the daemon
- **WHEN** a client that would replace the daemon connects during an active serial play
- **THEN** playback is aborted, the old daemon exits, a new daemon from that client starts, and the client's command completes against the new daemon

#### Scenario: Plain release still refuses during play
- **WHEN** a serial play is active and the user runs `daemon release` without `--yield`
- **THEN** the command fails with a play-active error, playback continues, and the daemon keeps the port

## REMOVED Requirements

### Requirement: Replace does not cut an active play
**Reason**: Host daemon replace is rare and more important than finishing an arbitrary WAV clip; refusing replace left callers on a stale daemon with no clear signal.
**Migration**: Expect replace (and install that replaces a live daemon) to abort play. Retry or re-issue `play` after upgrade if the clip must finish. Plain `daemon release` without yield still protects an active play.

## MODIFIED Requirements

### Requirement: Install hands the process to the supervisor
`daemon install` while a daemon is running MUST leave exactly one daemon, and that process MUST be one the supervisor started. If a serial play is active, install MUST still replace the running daemon (aborting that play) so the supervisor owns the next process.

#### Scenario: Install during play
- **WHEN** a serial play is active and the user runs `daemon install`
- **THEN** playback is aborted, install succeeds, and exactly one daemon remains under the supervisor

#### Scenario: Install replaces an orphan
- **WHEN** a daemon the CLI spawned is running and the user runs `daemon install`
- **THEN** one daemon remains and a later crash of that process is started again by the supervisor
