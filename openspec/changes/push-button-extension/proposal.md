# Proposal

## Why

The robot only accepts commands. A button on the extension port cannot drive host-side automation until the device can push events out, and that outbound path has to exist before audio streaming or further extensions need it.

## What Changes

- Add a firmware **outbound event hub**: producers publish events to two outbound transports, a WebSocket server and a serial `Evt` frame. REST stays the inbound control API. Serial stays request/response for commands; event frames are device-initiated only.
- WebSocket listens on port **81** while the HTTP server is running (Wi-Fi or setup AP). USB-only mode does not start it. The socket is outbound-only: client payloads are ignored. Connected clients receive a broadcast. There is no replay of past events.
- Discrete events use a versioned JSON text envelope. Binary frames are reserved for later audio streaming and are not sent in this change.
- Add a **push-button extension**, off by default. A settings boolean turns it on and off from a new web UI **Extensions** page (immediate, no reboot). When on, the firmware reads GPIO20 (J4 pin 4) with an interrupt latch (8-deep ring; overflow drops oldest) so typical clicks during `POST /play` are not lost, debounces them, and publishes `button.down` and `button.up`. When off, the pin is left idle and no button events are published.
- Extend the CLI with `watch`, which prints each event. Transport `http` (the default) uses the WebSocket and accepts only an `http:` robot base URL (no TLS on device port 81; `https:` is rejected). Transport `serial` reads `Evt` frames from the serial port.

No **BREAKING** HTTP route or query-param changes. The new settings field is additive and defaults off.

## Capabilities

### New Capabilities

- `outbound-events`: Device-initiated event bus, WebSocket and serial `Evt` outbound sinks (envelope, auth, lifetime), plus the CLI `watch` command that prints those events over the selected transport.
- `push-button`: Optional extension that reads one button on GPIO20 and publishes down/up events when enabled.

### Modified Capabilities

- (none)

## Impact

- **Firmware**: new event hub, WebSocket poll path next to the existing Arduino `WebServer`, and a serial `Evt` sink that sends the same JSON; push-button reader with ISR latch; one NVS boolean on `POST /settings`.
- **Web UI**: Extensions nav route and page; settings type includes the new field. The Config form does not show it.
- **CLI**: `tiny-engineer watch` in `packages/tiny-engineer-cli`, with tests. HTTP uses a small WebSocket client. Serial reuses the existing COBS client and port selection.
- **Docs**: `docs/api.md` (settings field, WebSocket, and serial `Evt` payload), `docs/settings.md`, UI API reference parameter list for `/settings`, pin note that GPIO20 is the push-button input when the extension is on.
- **Dependencies**: ESP32 WebSocket server library that can run beside `WebServer` on another port. No HTTP server migration.
