# Tasks

## 1. Protocol framing foundation

- [ ] 1.1 Vendor nanocobs (`cobs.c`/`cobs.h`) under `lib/` and add a native/host unit test that round-trips encode/decode for empty, sparse-zero, and max-chunk payloads
- [ ] 1.2 Implement COBS frame reader/writer on USB CDC with max payload caps and verify malformed input resyncs at the next `0x00` (unit or firmware test)
- [ ] 1.3 Implement typed frame header (type, ver, id, flags, len) plus JSON req/res/sync and LOG emission helpers; verify sync request returns version and provisioning state

## 2. Command core and HTTP adapter hookup

- [ ] 2.1 Introduce a transport-agnostic command core for anim, settings get/set/reset, health, hardware tests, servo test, and factory reset; verify existing HTTP routes still pass `pio run` and relevant native/UI tests
- [ ] 2.2 Replace Wi‑Fi-only `Access::Configured` gating with provisioned checks used by both HTTP and serial; verify unprovisioned HTTP still returns the documented not-configured/setup behavior
- [ ] 2.3 Add a global busy lock for play/long operations and verify a second concurrent play (HTTP or serial) returns busy without aborting the first

## 3. Connection mode and setup UI

- [ ] 3.1 Add NVS `link_mode` (`wifi`|`usb`), getters, migration (saved Wi‑Fi nets + unset mode → `wifi`), and factory-reset clearing; verify with `pio test -e native` settings coverage
- [ ] 3.2 Boot path: USB-only skips STA and does not start HTTP; Wi‑Fi mode keeps current connect/AP behavior; verify via boot logs / health fields once available
- [ ] 3.3 Extend setup wizard Network/Link step with USB-only choice; verify UI tests cover selecting USB-only and Wi‑Fi connect paths
- [ ] 3.4 Expose `link_mode` (and provisioning state) on `/auth` and/or `/settings`/`/health` as needed for UI/CLI; update `docs/api.md` and `ui/src/views/ApiReference.tsx` in the same change

## 4. Serial API operations and streaming

- [ ] 4.1 Dispatch full serial ops when provisioned; ping/sync only while provisioning; verify anim rejected in provisioning and accepted after USB-only provision
- [ ] 4.2 Implement credit-window stream play reusing `WavStreamParser` / speaker path; verify play completes for a short 16-bit mono 22050 WAV and credits prevent unbounded RX growth
- [ ] 4.3 Route `serialLog*` through LOG frames when `serial_log` is on; verify no raw application log lines on the wire in a framed session
- [ ] 4.4 Skip Bearer auth on serial; verify HTTP auth behavior unchanged when token set
- [ ] 4.5 Run `pio run` after firmware serial stack lands and fix compile failures

## 5. CLI transport, port cache, and parity commands

- [ ] 5.1 Add `--transport` / `TINY_ENGINEER_TRANSPORT` (default `http`, no cross-transport fallback) and verify CLI arg/env tests
- [ ] 5.2 Implement serial client (COBS + typed messages) with `--port` / `TINY_ENGINEER_PORT`, autodetect + cache + invalidate-on-fail; verify unit tests for cache hit/miss/stale
- [ ] 5.3 Refactor `anim`/`play`/`hook`/`doctor` onto a shared robot client interface; verify existing HTTP client tests still pass
- [ ] 5.4 Add CLI commands for settings get/set, tests, servo, health (if not covered), and factory reset with confirmation/`--yes`; verify command tests and help text
- [ ] 5.5 Add `tiny-engineer monitor` that prints LOG frame text; verify with a fixture frame stream
- [ ] 5.6 Wire agent packages (cursor/antigravity/claude-code) to honor transport via shared client or CLI; verify package tests with short timeouts / fail-open

## 6. Docs and integration check

- [ ] 6.1 Document serial protocol overview, connection mode, factory-reset escape, CLI transport/port/monitor, and PlatformIO monitor limitations in `docs/` (+ README touch if boot URLs/modes change)
- [ ] 6.2 End-to-end smoke: USB-only provision → CLI serial anim/play/settings/reset-confirm abort; Wi‑Fi mode → HTTP still works and serial still works; record results in the PR description

## Workflow follow-up

- Archive the change after review and implementation are complete (`/opsx-archive` or project equivalent).
- Optional later: PlatformIO custom monitor filter for LOG frames (`monitor_encoding=latin1`).
