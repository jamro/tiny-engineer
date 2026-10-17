# Spec Delta

## Purpose

Defines how the robot’s connection mode chooses whether Wi‑Fi and HTTP are enabled after setup, while keeping USB serial as an independent control path once provisioned.

## ADDED Requirements

### Requirement: Connection mode persisted at setup
The setup wizard’s final step MUST let the user choose either Wi‑Fi credentials (Wi‑Fi mode) or USB connection only (USB-only mode). The chosen mode MUST be stored persistently and applied on subsequent boots.

#### Scenario: Choose Wi-Fi at end of setup
- **WHEN** the user completes setup by connecting to a Wi‑Fi network
- **THEN** the device stores Wi‑Fi mode and leaves setup AP after a successful connection test (existing Wi‑Fi setup behavior)

#### Scenario: Choose USB-only at end of setup
- **WHEN** the user completes setup by selecting USB connection only while the setup AP is active
- **THEN** the device stores USB-only mode, does not join a Wi‑Fi network, stops the setup softAP after the HTTP response is sent, clears provisioning mode, and stops the HTTP control API for that session

#### Scenario: USB-only unlocks serial without reboot
- **WHEN** USB-only mode has just been saved and the setup AP has been torn down
- **THEN** the full serial API is available without requiring a power-cycle, and sync/health no longer report provisioning as active

### Requirement: Idle eyes after leaving setup AP
After the setup AP is torn down in-session (Wi‑Fi connect success or USB-only save), the OLED MUST leave the provisioning/status text and return to the normal idle eyes display. A brief status flash (IP, “No IP”, or USB-only ready) MAY appear first, matching cold-boot behavior, but MUST NOT remain indefinitely in place of eyes.

#### Scenario: Eyes after Wi-Fi setup leave
- **WHEN** the user completes Wi‑Fi setup and the setup AP has been stopped in the same session
- **THEN** after any brief IP (or “No IP”) status flash, the OLED shows idle eyes rather than leaving the status text on screen

#### Scenario: Eyes after USB-only setup leave
- **WHEN** the user completes USB-only setup and the setup AP has been stopped in the same session
- **THEN** after any brief USB-only status flash, the OLED shows idle eyes rather than leaving the status text on screen

### Requirement: Mode controls Wi-Fi and HTTP only
Connection mode MUST enable or disable Wi‑Fi station mode and the HTTP API. Connection mode MUST NOT disable the serial API once the device is provisioned.

#### Scenario: Wi-Fi mode enables HTTP
- **WHEN** the device boots in Wi‑Fi mode with valid credentials
- **THEN** Wi‑Fi station and the HTTP API are available after connect (subject to existing connect/fail-to-AP rules)

#### Scenario: USB-only mode disables HTTP
- **WHEN** the device boots in USB-only mode
- **THEN** the device does not start the HTTP control API and does not join Wi‑Fi as a station

#### Scenario: USB-only stops HTTP when leaving setup AP
- **WHEN** the user selects USB-only during an active setup AP session
- **THEN** after the successful save response, the HTTP control API is no longer served on the setup AP (AP stopped; no zombie config server)

#### Scenario: Serial remains in Wi-Fi mode
- **WHEN** the device is provisioned in Wi‑Fi mode
- **THEN** the full serial API remains available on USB CDC

### Requirement: Provisioned readiness without Wi-Fi
Operations that today require Wi‑Fi credentials to be configured MUST treat USB-only mode as provisioned. Those operations MUST be available over serial in USB-only mode without returning a Wi‑Fi-not-configured error. A power-cycle MAY still be recommended for a clean boot, but MUST NOT be required to end provisioning or use the full serial API after USB-only save.

#### Scenario: Anim in USB-only mode
- **WHEN** connection mode is USB-only and the host requests an animation over serial
- **THEN** the device does not reject the request solely because no Wi‑Fi networks are saved

#### Scenario: Anim after USB-only save in same power cycle
- **WHEN** the user has just completed USB-only setup and the setup AP is down
- **THEN** a serial animation request succeeds without a reboot

### Requirement: Escape USB-only via factory reset only
Leaving USB-only mode MUST require a factory reset that clears provisioning and reopens setup AP on the next boot cycle as today. The device MUST NOT offer a silent in-place switch from USB-only to Wi‑Fi mode without factory reset.

#### Scenario: Factory reset clears USB-only
- **WHEN** a factory reset is performed on a USB-only device
- **THEN** connection mode provisioning is cleared and setup AP mode becomes available again after reboot/power-cycle as documented for factory reset

### Requirement: link_mode patches gated after provision
Empty `link_mode` MUST remain valid as the unprovisioned NVS default and for settings load. Settings patches MUST reject an empty `link_mode` value. When the device is provisioned and not in setup AP mode, settings patches that change `link_mode` MUST be rejected over HTTP and serial. The setup AP Connection path MAY set `wifi` or `usb` during provisioning as specified above.

#### Scenario: Empty link_mode patch rejected
- **WHEN** a provisioned or unprovisioned device receives a settings patch with `link_mode` set to an empty string
- **THEN** the device rejects the patch with a validation error and does not clear connection mode

#### Scenario: Provisioned mode change outside setup AP rejected
- **WHEN** the device is provisioned in Wi‑Fi mode, setup AP is not active, and the host patches `link_mode` to `usb`
- **THEN** the device rejects the patch and leaves the saved mode unchanged

#### Scenario: USB-only cannot switch to Wi-Fi via settings
- **WHEN** the device is provisioned in USB-only mode and the host patches `link_mode` to `wifi` without a factory reset
- **THEN** the device rejects the patch

#### Scenario: Setup AP USB-only save still works
- **WHEN** the setup AP is active and the user completes USB-only Connection
- **THEN** the device accepts and stores `link_mode=usb` and performs the in-session AP teardown as specified

### Requirement: Factory reset over CLI with confirmation
The host CLI MUST expose factory reset over the selected transport and MUST require an explicit confirmation step (interactive prompt or affirmative flag) before sending the reset command.

#### Scenario: Reset aborted without confirmation
- **WHEN** the user runs factory reset without confirming
- **THEN** the CLI does not send a reset command to the device

#### Scenario: Confirmed reset
- **WHEN** the user confirms factory reset in the CLI
- **THEN** the CLI sends the factory reset operation and the device performs factory reset
