# Design

## Context

HTTP is an Arduino `WebServer` on port 80 (`src/http/server.cpp`). It starts only when Wi-Fi is connected or the setup AP is up, and `stopHttpServer()` runs when setup saves USB-only. `POST /play` is handled inside `server.handleClient()`, so `loop()` does not run for the whole body. Serial already defines an `Evt` message type and requires event payloads to be JSON; nothing emits those frames yet. Settings are a flat JSON object plus a short NVS key; `serial_log` is the boolean pattern. The CLI selects `http` or `serial` by flag, then env, then default `http`, and has no WebSocket dependency (`engines` node >= 18).

See proposal.md for why. Behavior contracts are the `outbound-events` and `push-button` deltas.

## Goals / Non-Goals

**Goals:**

- One outbound hub that later producers (audio) can publish into without opening their own socket.
- Button edges survive a blocked `loop()` during `POST /play`.
- The extension toggle follows the existing settings stack, with the control only on the Extensions page.

**Non-Goals:**

- Inbound commands over WebSocket or over serial `Evt` frames.
- Audio streaming, binary frames on the wire, event history, or more than one button.
- Moving HTTP onto an async server.

## Decisions

### Hub beside `WebServer`, port 81

Use `links2004/WebSockets` `WebSocketsServer` on port 81. Poll it from `pollHttpServer()` and stop it inside `stopHttpServer()`, so the hub exists exactly while HTTP does (Wi-Fi and setup AP; not USB-only).

Reject the HTTP upgrade unless the path is `/events` and, when `settingsAccessTokenSet()`, the `token` query matches the access token with the same constant-time compare as Bearer auth. Do not log the request URI or the token. Cap clients at 4; further handshakes fail.

Alternative: migrate to `ESPAsyncWebServer` so WebSocket shares port 80. Rejected — it rewrites every route.

### Text events now, binary hook later

`publishEvent(type, source)` writes one ArduinoJson object `{v:1, type, source, ms:millis()}` once. It `broadcastTXT`s that text and packs the same bytes as a serial `Evt` frame (`SerialMsgType::Evt`). The serial write is best-effort: if the CDC transmit buffer cannot take the frame, drop that copy and do not block. WebSocket delivery does not depend on the serial write. Producers call this from loop context, never from an ISR. A `publishBinary` entry point is declared and not called, so a later audio producer does not add a second socket. No binary frames are sent.

Serial has no Bearer check, matching the rest of the serial API. USB-only mode has no WebSocket server; the `Evt` sink still runs.

Client text and binary callbacks do nothing to robot state. Ping/pong and close stay library defaults.

### Button is a producer, not a transport

`push_button` is a settings boolean, NVS key `ext_btn`, default false, same layers as `serial_log` (`docs/settings.md`). Apply is immediate: after load and after a successful save, `pushButtonApply(settingsPushButton())` runs.

On: `pinMode(20, INPUT_PULLUP)`, `attachInterrupt` on `CHANGE`, sample the level, and remember it without publishing. Off: `detachInterrupt`, `pinMode(20, INPUT)` with no pull-up, drop queued edges, publish nothing. Pin constant lives in `include/pins.h`.

The ISR (`IRAM_ATTR`) only pushes `{level, esp_timer_get_time()}` into an 8-deep ring guarded by a portMUX. `pushButtonPoll()` runs from `loop()` and from the `POST /play` raw body callbacks. It accepts an edge when the level differs from the last accepted level and at least 25 ms have passed since that level was accepted. Accepted edges call `publishEvent("button.down"|"button.up", "push-button")`. A click during play is two queued edges and is published on the next poll, including a poll from the play callbacks.

The pure accept/debounce step is a function with no GPIO, covered by `pio test -e native`.

### UI exception to the settings checklist

`docs/settings.md` says new fields get a Config form control. This field does not. Add it to `Settings` and the API reference parameter list, and put the only control on a new `/extensions` route in the main nav. The page posts `{push_button: boolean}` and shows J4 pin 4, active-low, internal pull-up, other side to GND.

### CLI `watch`

Add the `ws` package (Node 18 has no global `WebSocket`). `watch` uses the same transport resolution as other robot commands. On `http`, rewrite the robot base URL from `http` to `ws` and `https` to `wss`, force port 81 and path `/events`, and append `token` when the HTTP client would send Bearer. On `serial`, open the port with the existing override and autodetect path, decode COBS frames, and print only `Evt` payloads. Other frame types are ignored, not dumped as binary. Each event is one line. Exit non-zero if the selected link cannot open. Serial `watch` does not need `TINY_ENGINEER_URL`. Tests cover HTTP URL building, token redaction, and serial frame selection without a device.

## Risks / Trade-offs

- [Clicks shorter than 25 ms look like bounce] → Acceptable for a finger. The window is one constant.
- [Ring of 8 drops the oldest edges if the user mashes the button through a long play] → One click is two edges. Document the cap; do not block the ISR.
- [Query token can land in a proxy log on the host] → Device logs must not print it. Same secret model as Bearer, which browsers cannot set on a WebSocket.
- [Four clients] → Enough for a UI tab plus a CLI watch. A fifth handshake fails instead of growing RAM on the C3.
- [`links2004` header hook might not see the URI] → Implementation must fail the upgrade before `101`. If that hook cannot, stop and switch the hub to an IDF `httpd` on port 81 rather than disconnecting after accept.
- [CDC TX full during a burst drops serial copies] → WebSocket clients still get the event. Do not wait on `Serial.write`.
- [Play callbacks must poll] → Forgetting the call site drops the "click during play" scenario. Task calls it out next to `handlePlayBody`. The same poll sends both sinks.

## Migration Plan

New NVS key. Missing key reads as false, so existing devices stay off. Factory reset already writes defaults. Older firmware ignores an unknown key. No HTTP route is removed. Rollback is reverting the firmware; a saved `ext_btn` value is harmless to an older build.

## Open Questions

None.
