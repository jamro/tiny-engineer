# Spec Delta

## Purpose

Defines the USB CDC serial application protocol so hosts can control the robot with the same capabilities as the HTTP API, including streamed WAV playback, without relying on Wi‑Fi.

## ADDED Requirements

### Requirement: COBS-framed typed messages
The device MUST exchange application messages with the host over USB CDC using COBS framing with a `0x00` delimiter. Each decoded frame MUST carry a typed application message of one of: request, response, log, event, or stream.

#### Scenario: Host sends a framed request
- **WHEN** the host sends a valid COBS-encoded request frame for a supported operation
- **THEN** the device decodes the frame and processes that operation

#### Scenario: Delimiter separates frames
- **WHEN** two COBS frames are sent back-to-back on the wire
- **THEN** the receiver treats each `0x00`-terminated segment as a separate frame

### Requirement: Protocol version and correlation
Every request and response MUST include a protocol version and a correlation identifier. The device MUST reject unsupported protocol versions with an error response that includes the correlation identifier of the rejected request.

#### Scenario: Matching request and response
- **WHEN** the host sends a request with correlation id `N`
- **THEN** the device response for that request uses the same correlation id `N`

#### Scenario: Unsupported version
- **WHEN** the host sends a request with an unsupported protocol version
- **THEN** the device responds with an error indicating version mismatch and does not perform the operation

### Requirement: Resynchronization after malformed input
After a decode failure or unexpected bytes, the receiver MUST discard input until the next COBS delimiter and resume framing. The protocol MUST provide a sync exchange so either side can confirm the link is healthy after resync.

#### Scenario: Noise then valid frame
- **WHEN** non-frame bytes appear on the wire followed by a valid COBS frame
- **THEN** the receiver ignores the noise and successfully decodes the valid frame

#### Scenario: Sync after disruption
- **WHEN** the host sends a sync request after a framing error
- **THEN** the device replies with a sync response including protocol version and provisioning state

### Requirement: JSON for control messages
Request, response, log, and event payloads MUST use JSON. Stream frames that carry audio MUST use raw binary payloads (not Base64).

#### Scenario: Animation request
- **WHEN** the host sends a JSON request to set an animation
- **THEN** the device applies the animation and returns a JSON response

#### Scenario: WAV stream uses binary
- **WHEN** the host streams WAV audio over serial
- **THEN** audio bytes are sent in binary stream frames without Base64 encoding

### Requirement: Audio stream flow control
During WAV (or PCM) streaming, the device MUST apply credit-based flow control so the host does not send more outstanding stream payload than the device advertises. The device MUST refuse to accept stream data that would exceed its advertised credits.

#### Scenario: Credits gate chunks
- **WHEN** the host has exhausted stream credits
- **THEN** the host MUST wait for additional credits before sending further stream payload frames

#### Scenario: Successful play completes
- **WHEN** the host opens a play stream, sends WAV data within credits, and closes the stream
- **THEN** the device plays the audio and returns a successful play completion response

### Requirement: CDC buffers sized for stream frames
The device MUST size USB CDC receive (and transmit) buffers so that one maximum legal stream DATA frame, including COBS overhead and delimiter, fits without RX overflow under normal `serialLinkPoll` latency. Advertised play credits MUST NOT imply a single host chunk larger than that capacity. When the device rejects a stream DATA chunk, it MUST inform the host (error response, event, or equivalent) rather than remaining silent until the host credit wait times out.

#### Scenario: Play credits complete under healthy link
- **WHEN** the host opens a play stream and sends WAV chunks within advertised credits over a healthy USB link
- **THEN** the device returns stream credit frames for accepted chunks and play completes without the host timing out waiting for credits

#### Scenario: Rejected stream chunk is not silent
- **WHEN** the device refuses a stream DATA chunk (bad stream id, over credit, or parse failure)
- **THEN** the host receives an explicit failure indication instead of only a credit-wait timeout

### Requirement: Application logs as LOG frames
When serial logging is enabled, application diagnostic log lines MUST be emitted as typed log frames. The device MUST NOT emit those application log lines as raw unframed text on USB CDC.

#### Scenario: Serial log enabled
- **WHEN** `serial_log` is enabled and the firmware emits an application log line
- **THEN** the host receives that line inside a typed log frame

#### Scenario: Serial log disabled
- **WHEN** `serial_log` is disabled
- **THEN** the device does not emit application log frames for routine diagnostics

### Requirement: Serial API availability when provisioned
After the device is provisioned (a connection mode has been chosen), the full serial API MUST remain available whether Wi‑Fi/HTTP is enabled or not. Serial requests MUST NOT require HTTP Bearer authentication.

#### Scenario: Full API in Wi-Fi mode
- **WHEN** connection mode is Wi‑Fi and the host sends a serial animation request
- **THEN** the device performs the animation without requiring a Bearer token

#### Scenario: Full API in USB-only mode
- **WHEN** connection mode is USB-only and the host sends a serial health request
- **THEN** the device returns health information over serial

### Requirement: Ping-only during provisioning
While the device is unprovisioned or in setup AP provisioning, the serial API MUST accept only sync/ping (identity) operations and MUST reject other control operations with a clear error.

#### Scenario: Ping during setup AP
- **WHEN** the setup AP is active and the host sends a sync/ping request
- **THEN** the device responds with identity information indicating provisioning state

#### Scenario: Anim rejected while provisioning
- **WHEN** the setup AP is active and the host sends a serial animation request
- **THEN** the device rejects the request without changing animation state

### Requirement: Feature parity with HTTP control surface
The serial API MUST expose the same day-to-day control operations as the HTTP JSON API, including animations, settings read/write, hardware tests, servo test, health, factory reset, and streamed WAV play with optional animation name.

#### Scenario: Settings over serial
- **WHEN** the host requests current settings over serial
- **THEN** the device returns the same settings fields exposed by `GET /settings` (excluding secrets)

#### Scenario: Play with pose over serial
- **WHEN** the host streams a WAV with an allowed play animation name over serial
- **THEN** the device plays the clip while running that animation, matching `POST /play` behavior

### Requirement: Future device-initiated events
The protocol MUST reserve and define an event message type so the device can later send unsolicited notifications to the host without changing the framing layer.

#### Scenario: Event type recognized
- **WHEN** a valid event frame is decoded
- **THEN** the host protocol stack accepts it as an event even if no event producers exist yet on the device

### Requirement: Concurrent transport busy handling
If a long-running operation (such as WAV play) is active on one transport, a conflicting operation on the other transport MUST fail with a busy error rather than corrupting the in-progress operation.

#### Scenario: Second play while busy
- **WHEN** a play stream is active over serial and the host starts another play over HTTP
- **THEN** the second play is rejected with a busy error and the first play continues

### Requirement: Play session released on stream failure
When an active serial play session rejects stream DATA (bad stream id, over credit, WAV parse failure, or equivalent), the device MUST end that session: release the play busy lock, clear session state, and restore the prior animation if playback had already started. The device MUST NOT remain busy solely until a later `play_end` or reboot.

#### Scenario: Parse failure releases busy
- **WHEN** an active serial play session rejects a DATA chunk because WAV parsing failed
- **THEN** the host receives an explicit failure indication and a subsequent play request is not rejected solely because the previous session left the busy lock held

#### Scenario: Second play after failed feed
- **WHEN** a serial play feed has failed and the session was aborted
- **THEN** a new play request on serial or HTTP may start (subject to normal validation) without requiring reboot
