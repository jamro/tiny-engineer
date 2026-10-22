# Tasks

## 1. Push-button setting

- [x] 1.1 Add `push_button` (NVS `ext_btn`, default false) through the settings layers in `docs/settings.md` and `GET`/`POST /settings`, and verify `pio test -e native` still passes
- [x] 1.2 Document `push_button` in `docs/api.md`, `docs/settings.md`, and the `GET /settings` field list in `docs/hardware/testing.md`, and verify each lists the boolean, default false, and immediate apply

## 2. Outbound event hub

- [x] 2.1 Add `links2004/WebSockets` and a port-81 `/events` hub that starts and stops with the HTTP server, rejects upgrades that fail path or token checks, ignores client payloads, and broadcasts text events, and verify `pio run` compiles
- [x] 2.2 Send each published event as a serial `Evt` frame with the same JSON, drop the serial copy instead of blocking when the CDC buffer is full, and verify `pio run` compiles
- [x] 2.3 Document `ws://<host>:81/events` and the matching serial `Evt` payload (envelope `v`, `type`, `source`, `ms`; query `token` on WebSocket only; no binary frames) in `docs/api.md`, and verify the section matches the `outbound-events` spec

## 3. Push-button producer

- [x] 3.1 Add a GPIO-free debounce/queue step (25 ms, depth 8, no event on the enable sample) and a native test for press/release, bounce, and a click queued while polling is paused, and verify `pio test -e native` passes
- [x] 3.2 Wire GPIO20 (`include/pins.h`) with an ISR latch, `pushButtonApply` on boot and on save, and `pushButtonPoll` from `loop()` and from `POST /play` raw callbacks, and verify `pio run` compiles
- [x] 3.3 Note on the GPIO20 row in `docs/hardware/pinout.md` that the push-button extension uses J4 pin 4, active-low, internal pull-up, and verify the note is present

## 4. Extensions page

- [x] 4.1 Add `/extensions` to the nav and a page whose only push-button control posts `{push_button: boolean}`, keep that control off `ConfigForm`, add the field to `ui/src/api/types.ts` and the `/settings` parameter list in `ui/src/views/ApiReference.tsx`, and verify `npm run lint --prefix ui` and `npm test --prefix ui` pass

## 5. CLI watch

- [x] 5.1 Add the `ws` dependency and `tiny-engineer watch` (transport flag, then `TINY_ENGINEER_TRANSPORT`, then `http`; HTTP uses port 81 path `/events` and a token query when Bearer would be sent; serial opens the existing port path, prints `Evt` JSON only, and does not require a URL; one JSON line per event; non-zero exit if the selected link cannot open), with unit tests for URL building, token redaction, and serial frame selection, and verify `npm test --prefix packages/tiny-engineer-cli` passes

## 6. Integration

- [x] 6.1 Run `pio run` after the firmware groups and verify the default env links
