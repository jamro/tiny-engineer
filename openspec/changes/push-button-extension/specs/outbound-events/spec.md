# Spec Delta

## Purpose

Lets the robot push versioned events to hosts over WebSocket and USB serial, and lets the CLI print those events on the selected transport, without using either path for inbound control.

## ADDED Requirements

### Requirement: WebSocket hub lifetime follows HTTP
The device MUST accept outbound WebSocket connections on port 81 at path `/events` while the HTTP control server is running, and MUST NOT listen on that port when the HTTP control server is not running.

#### Scenario: Wi-Fi mode serves the hub
- **WHEN** the device is provisioned in Wi-Fi mode and the HTTP server is running
- **THEN** a client can open a WebSocket to port 81 path `/events`

#### Scenario: USB-only mode has no hub
- **WHEN** the device boots in USB-only mode
- **THEN** nothing accepts WebSocket connections on port 81

#### Scenario: Setup AP serves the hub
- **WHEN** the setup AP is active and the HTTP server is running
- **THEN** a client on that AP can open a WebSocket to port 81 path `/events`

### Requirement: WebSocket is outbound only
The hub MUST ignore client text and binary payloads. It MUST NOT change robot state because a client sent data. Closing the socket and WebSocket ping/pong MUST still work.

#### Scenario: Client text is not a command
- **WHEN** a connected client sends a text frame
- **THEN** the device does not change animation, settings, or playback, and the socket stays open

### Requirement: Broadcast without replay
Each published event MUST be sent to every client connected at publish time. A client that connects later MUST NOT receive events published before it connected.

#### Scenario: Two clients see the same event
- **WHEN** two clients are connected and a producer publishes one event
- **THEN** both clients receive that event once

#### Scenario: Late client misses the event
- **WHEN** an event is published and a client connects afterward
- **THEN** that client does not receive the earlier event

### Requirement: Discrete events use a versioned JSON envelope
Each discrete event MUST be one UTF-8 JSON text frame with number `v` equal to 1, string `type`, string `source`, and number `ms` (milliseconds since boot). The hub MUST NOT send binary frames in this change.

#### Scenario: Text envelope
- **WHEN** a producer publishes a discrete event
- **THEN** each connected client receives one JSON text frame containing `v`, `type`, `source`, and `ms`

### Requirement: WebSocket auth matches the access token
When an access token is set, the hub MUST accept the connection only if the query parameter `token` equals that token, and MUST reject otherwise. When no access token is set, the hub MUST accept a connection with no token. The device MUST NOT log the token value.

#### Scenario: Matching token
- **WHEN** an access token is set and the client connects with that token as `token`
- **THEN** the WebSocket handshake succeeds

#### Scenario: Missing or wrong token
- **WHEN** an access token is set and `token` is missing or different
- **THEN** the handshake fails and the client receives no events

#### Scenario: Open when no token is configured
- **WHEN** no access token is set and the client connects without `token`
- **THEN** the handshake succeeds

### Requirement: Serial event frames carry the same envelope
Each published discrete event MUST also be sent as one serial `Evt` frame whose JSON payload is the same object as the WebSocket text frame. A serial write that cannot be queued MUST NOT block the device and MUST NOT drop the WebSocket copy. Serial event frames MUST NOT require a Bearer token.

#### Scenario: Same JSON on serial
- **WHEN** a producer publishes a discrete event and the serial link can accept the frame
- **THEN** the host receives one `Evt` frame whose payload equals the JSON sent to WebSocket clients

#### Scenario: Full serial buffer does not stall
- **WHEN** a producer publishes an event and the serial transmit buffer cannot take the frame
- **THEN** the device continues and still sends that event to connected WebSocket clients

#### Scenario: USB-only still emits serial events
- **WHEN** the device is in USB-only mode and a producer publishes an event
- **THEN** the event is sent as a serial `Evt` frame

### Requirement: CLI watch follows transport selection
`tiny-engineer watch` MUST follow CLI transport selection: flag, then `TINY_ENGINEER_TRANSPORT`, then default `http`.

#### Scenario: Flag selects serial
- **WHEN** the user runs `watch` with `--transport serial`
- **THEN** the command opens the serial port and does not open a WebSocket

#### Scenario: Default is HTTP
- **WHEN** no transport flag and no `TINY_ENGINEER_TRANSPORT` are set
- **THEN** `watch` uses the WebSocket

### Requirement: CLI watch over HTTP uses the event socket
On transport `http`, `watch` MUST connect only when the robot base URL uses the `http:` scheme. It MUST open `ws://<host>:81/events` and append `token` when a Bearer token would be sent on HTTP. Command output MUST NOT contain the token. When the base URL uses `https:`, `watch` MUST exit non-zero with an error and MUST NOT open `ws://` or `wss://` (the device has no TLS listener on port 81).

#### Scenario: HTTP watch prints lines
- **WHEN** transport is `http`, the base URL is `http:`, and the device publishes an event
- **THEN** the command prints that event's JSON as a single line and keeps waiting

#### Scenario: HTTPS base URL is rejected
- **WHEN** transport is `http` and the robot base URL uses `https:`
- **THEN** the command exits non-zero, prints an error, and does not open a WebSocket

#### Scenario: Token is not printed
- **WHEN** `watch` connects over HTTP with an access token
- **THEN** the command output does not contain the token value

### Requirement: CLI watch over serial reads event frames
On transport `serial`, `watch` MUST open the serial port the same way as other serial commands and MUST NOT require a robot URL. It MUST print each `Evt` JSON payload as one line and MUST NOT print non-event frames as raw binary.

#### Scenario: Serial watch prints event frames
- **WHEN** transport is `serial` and the device sends an `Evt` frame
- **THEN** the command prints that frame's JSON as a single line and does not open a WebSocket

#### Scenario: Serial watch ignores other frames
- **WHEN** transport is `serial` and the device sends a log or response frame
- **THEN** the command does not print that frame as raw binary

#### Scenario: Serial watch needs no URL
- **WHEN** transport is `serial` and no robot HTTP URL is configured
- **THEN** `watch` still opens the serial port and does not fail for a missing URL

### Requirement: CLI watch exits when the link cannot open
`watch` MUST exit non-zero and print an error when the selected link cannot be opened. It MUST keep running after a successful open until the link closes or the user interrupts it.

#### Scenario: Unreachable robot
- **WHEN** the user runs `watch` and the selected link cannot be opened
- **THEN** the command exits non-zero and prints an error
