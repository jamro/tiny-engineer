# Spec Delta

## MODIFIED Requirements

### Requirement: Hooks and doctor respect transport
Hook posting, doctor health checks, and other robot-facing CLI entry points MUST use the same explicit transport configuration as `anim` / `play`. When that transport is `serial`, they MUST use the host daemon, including auto-start, so a running serial `watch` or `monitor` does not make the call fail because the port is busy. They MUST NOT auto-start while a disable or a yield is active. A serial command in that window MUST fail with that error and MUST NOT start a daemon.

#### Scenario: Doctor over serial
- **WHEN** transport is `serial` and the user runs doctor
- **THEN** doctor probes health over serial and does not require HTTP reachability

#### Scenario: Hook over serial
- **WHEN** transport is `serial` and a hook maps to an animation
- **THEN** the CLI posts that animation over serial with the existing fail-open timing behavior for offline robots

#### Scenario: Hook while watch holds serial
- **WHEN** transport is `serial`, `watch` is running, and a hook maps to an animation
- **THEN** the hook posts that animation through the host daemon instead of failing because the port is open

#### Scenario: Hook during disable
- **WHEN** transport is `serial`, a disable is set, and a hook maps to an animation
- **THEN** the hook fails, names the disable, and does not start a daemon

#### Scenario: Hook during yield
- **WHEN** transport is `serial`, a yield is active, and a hook maps to an animation
- **THEN** the hook fails, names the yield, and does not start a daemon
