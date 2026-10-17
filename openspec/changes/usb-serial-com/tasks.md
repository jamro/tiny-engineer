# Tasks

## 1. Protocol framing foundation

- [x] 1.1 Vendor nanocobs (`cobs.c`/`cobs.h`) under `lib/` and add a native/host unit test that round-trips encode/decode for empty, sparse-zero, and max-chunk payloads
- [x] 1.2 Implement COBS frame reader/writer on USB CDC with max payload caps and verify malformed input resyncs at the next `0x00` (unit or firmware test)
- [x] 1.3 Implement typed frame header (type, ver, id, flags, len) plus JSON req/res/sync and LOG emission helpers; verify sync request returns version and provisioning state

## 2. Command core and HTTP adapter hookup

- [x] 2.1 Introduce a transport-agnostic command core for anim, settings get/set/reset, health, hardware tests, servo test, and factory reset; verify existing HTTP routes still pass `pio run` and relevant native/UI tests
- [x] 2.2 Replace Wi‑Fi-only `Access::Configured` gating with provisioned checks used by both HTTP and serial; verify unprovisioned HTTP still returns the documented not-configured/setup behavior
- [x] 2.3 Add a global busy lock for play/long operations and verify a second concurrent play (HTTP or serial) returns busy without aborting the first

## 3. Connection mode and setup UI

- [x] 3.1 Add NVS `link_mode` (`wifi`|`usb`), getters, migration (saved Wi‑Fi nets + unset mode → `wifi`), and factory-reset clearing; verify with `pio test -e native` settings coverage
- [x] 3.2 Boot path: USB-only skips STA and does not start HTTP; Wi‑Fi mode keeps current connect/AP behavior; verify via boot logs / health fields once available
- [x] 3.3 Extend setup wizard Network/Link step with USB-only choice; verify UI tests cover selecting USB-only and Wi‑Fi connect paths
- [x] 3.4 Expose `link_mode` (and provisioning state) on `/auth` and/or `/settings`/`/health` as needed for UI/CLI; update `docs/api.md` and `ui/src/views/ApiReference.tsx` in the same change
- [x] 3.5 After USB-only save during setup AP: mirror Wi‑Fi teardown (reply → delay → stop AP → clear provisioning → stop HTTP); unlock full serial without reboot; OLED + UI copy (no mandatory power-cycle); cover with firmware/UI test
- [x] 3.6 After in-session AP teardown (Wi‑Fi leave and USB-only leave): brief status then `showIdleScreen()` / eyes (same as boot IP hold → idle); do not leave IP / “No IP” / USB-only text stuck; share helper with boot if practical; verify on device or with a focused test hook

## 4. Serial API operations and streaming

- [x] 4.1 Dispatch full serial ops when provisioned; ping/sync only while provisioning; verify anim rejected in provisioning and accepted after USB-only provision
- [x] 4.2 Implement credit-window stream play reusing `WavStreamParser` / speaker path; verify play completes for a short 16-bit mono 22050 WAV and credits prevent unbounded RX growth
- [x] 4.3 Route `serialLog*` through LOG frames when `serial_log` is on; verify no raw application log lines on the wire in a framed session
- [x] 4.4 Skip Bearer auth on serial; verify HTTP auth behavior unchanged when token set
- [x] 4.5 Run `pio run` after firmware serial stack lands and fix compile failures
- [x] 4.6 HWCDC sizing for stream play: raise RX/TX before `Serial.begin` (recommend 4096); ensure `kSerialPlayCreditChunk` + CLI max chunk fit under RX with COBS margin; on `serialPlayFeed` failure surface error (no silent credit stall); verify CLI `play assets/test/audio_test_short_22K.wav --transport serial` completes on device

## 5. CLI transport, port cache, and parity commands

- [x] 5.0 Extend `tiny-engineer setup` (and uninstall `--purge-env`) so interactive/non-interactive flows choose host transport, merge `TINY_ENGINEER_TRANSPORT` / optional `TINY_ENGINEER_PORT` into project `.env`, keep URL merge for HTTP, and verify setup/uninstall tests cover serial and HTTP paths
- [x] 5.1 Add `--transport` / `TINY_ENGINEER_TRANSPORT` resolution (`flag > env > default http`, no cross-transport fallback) and verify CLI arg/env tests including setup-written `.env`
- [x] 5.2 Implement serial client (COBS + typed messages) with `--port` / `TINY_ENGINEER_PORT`, autodetect + cache + invalidate-on-fail; verify unit tests for cache hit/miss/stale
- [x] 5.3 Refactor `anim`/`play`/`hook`/`doctor` onto a shared robot client interface; verify existing HTTP client tests still pass
- [x] 5.4 Add CLI commands for settings get/set, tests, servo, health (if not covered), and factory reset with confirmation/`--yes`; verify command tests and help text
- [x] 5.5 Add `tiny-engineer monitor` that prints LOG frame text; verify with a fixture frame stream
- [x] 5.6 Wire agent packages (cursor/antigravity/claude-code) to honor transport via shared client or CLI; verify package tests with short timeouts / fail-open

## 6. Docs and integration check

- [x] 6.1 Document serial protocol overview, device Connection (`link_mode`) vs project setup (host transport), factory-reset escape, CLI transport/port/monitor, and PlatformIO monitor limitations in `docs/` (+ README touch if boot URLs/modes change)
- [x] 6.2 End-to-end smoke: USB-only provision → setup AP drops and serial anim works without power-cycle → CLI serial **play** (short 22050 WAV) / settings / reset-confirm abort after 4.6; Wi‑Fi mode → HTTP still works and serial still works; record results in the PR description

## 7. Review follow-ups (P0–P1)

- [x] 7.1 Play session abort on stream failure: firmware releases busy/session (restore anim if started) when feed/parse rejects DATA; CLI serial `play` best-effort `play_end` in `finally` while port open; cover with unit/CLI test where practical
- [x] 7.2 `link_mode` patch gates: reject empty in patch validation (NVS load still allows `""`); reject changed `link_mode` when provisioned and not in setup AP (HTTP + serial / `apiSaveSettings`); keep setup AP USB-only path; `pio test -e native` coverage
- [x] 7.3 Move large serial `message` / `decoded` buffers off loop-task stack (`static` or file-scope); `pio run`

## Workflow follow-up

- Archive the change after review and implementation are complete (`/opsx-archive` or project equivalent).
- Optional later: PlatformIO custom monitor filter for LOG frames (`monitor_encoding=latin1`).
- Deferred (not in this update): serial settings unknown-key / full HTTP field parity; `reboot_required` for `link_mode`; play idle timeout; OpenSpec sync RENAMED-before-MODIFIED doc fix.
