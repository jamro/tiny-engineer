# Spec Delta

## MODIFIED Requirements

### Requirement: PlatformIO pre-upload parks over serial
Firmware upload via PlatformIO MUST attempt a pre-upload park by invoking the CLI park command with serial transport. After that attempt, whether park succeeded or failed, the pre-upload step MUST request a host-daemon stop with yield so serial open stays refused and the serial port is free for the flash tool. If park fails (no port, timeout, busy CDC, cold board, missing Node/CLI) or the yield request fails, upload MUST continue after a clear warning.

#### Scenario: Board online before upload
- **WHEN** `pio run -t upload` runs and the robot answers serial park
- **THEN** joints are parked before the flash reset proceeds

#### Scenario: Board offline before upload
- **WHEN** `pio run -t upload` runs and park cannot complete
- **THEN** the pre-upload step warns and the upload continues

#### Scenario: Daemon was holding the port
- **WHEN** `pio run -t upload` runs while the host daemon holds the serial port
- **THEN** the pre-upload step yields that daemon, the daemon process is still running, and the flash tool is not blocked by the daemon

#### Scenario: Yield does not reclaim the port
- **WHEN** the flash tool still holds the serial port after the yield window and serial `watch` is still connected
- **THEN** the daemon process is still running and does not open the port
