# Design

## Context

See proposal.md for motivation. Today control is HTTP-only (`src/http/`, `Access::Configured` gated on `settingsWifiConfigured()`). USB CDC (`Serial` @ 115200, `ARDUINO_USB_CDC_ON_BOOT=1`) is flash + optional `serialLog*` text. CLI (`packages/tiny-engineer-cli`) talks HTTP via `fetch`; setup wizard ends at Wi‑Fi (`ui/src/setup/NetworkStep.tsx`). `/play` streams WAV into `WavStreamParser` (22050 Hz mono; 256-sample batches) while blocking `loop()` and ticking the robot.

Constraints: ESP32-C3 heap (~120 KB free typical); same CDC for protocol + logs; PlatformIO monitor is text-oriented; agent hooks must stay fail-open and fast.

## Goals / Non-Goals

**Goals:**

- Shared command core behind HTTP and serial adapters
- COBS + typed messages (JSON control, binary stream) with version, ids, resync, credits
- `link_mode` gates Wi‑Fi/HTTP only; serial full API when provisioned
- CLI explicit transport (default HTTP), serial port autodetect+cache, UI parity commands, LOG monitor
- Lightweight: vendored COBS, reuse ArduinoJson, no new heavy frameworks

**Non-Goals:**

- BLE, second UART, or alternate physical links
- Leaving USB-only without factory reset
- Auto-picking HTTP vs serial from probes
- Replacing PlatformIO monitor as primary (CLI monitor is primary; PIO filter optional later)
- Implementing rich device→host events in v1 beyond the event type + empty producers
- Replicating setup-only calibration (`/setup/*`) in CLI (stays AP web UI)

## Decisions

### D1: Command core + thin transports

Extract operations (anim, settings, tests, play session, health, factory reset) into a transport-agnostic core. HTTP handlers and serial request dispatcher call the same core. Refusal rules become “provisioned?” / “setup?” / “busy?” rather than HTTP-only concerns; Bearer applies only in the HTTP adapter.

**Alternatives:** Duplicate logic per transport (rejected — drift); HTTP-over-serial literal (rejected — awkward binary WAV + auth headers).

### D2: Wire format — header + payload inside COBS

Each COBS frame (delimited by `0x00`) carries:

```
[type:u8][ver:u8][id:u16 LE][flags:u8][len:u16 LE][payload...]
```

| type | payload |
| --- | --- |
| req / res / log / evt / sync | UTF-8 JSON |
| stream | binary header (stream_id, seq, kind) + raw bytes |

Protocol version starts at `1`. Max payload capped (≤2048 decode; stream chunk sized so framed COBS fits under HWCDC RX — see D3; 512–1024 B after RX enlarge is fine) to bound decode buffers.

**COBS library:** vendor [nanocobs](https://github.com/charlesnicholson/nanocobs) (`cobs.c`/`cobs.h`) — C99, no libc, incremental APIs, ~1–1.5 KB flash. Host: npm `nanocobs` or same algorithm in JS for byte identity.

**Alternatives:** PatrickBaus (254-byte only — reject); ESPP COBS (`std::vector` — reject); SLIP (similar, less tooling match).

### D3: Stream flow control

Play opens a stream; device advertises a credit window; host sends stream DATA only while credits > 0; device returns credits after draining a chunk (aligned with `writeMonoToSpeaker` blocking). Reuse `WavStreamParser` sink path from HTTP play. One outstanding DATA chunk is enough; credits alone do not enlarge the USB RX queue.

**HWCDC buffer constraint (ESP32-C3, `ARDUINO_USB_MODE=1`):** Arduino HWCDC defaults **RX/TX = 256 B** unless raised before `Serial.begin`. A framed stream DATA message is `7 (header) + 4 (stream hdr) + chunk` then COBS (~chunk+11 worst-case). With the historical 1024 B credit chunk that is ~1041 B on the wire — larger than the default RX queue. ISR enqueue then overflows while `loop()` is elsewhere → COBS never completes → no Stream Credit → CLI `serial credit timed out` even though play JSON start succeeded.

**Required sizing:**

1. Before `Serial.begin`: `Serial.setRxBufferSize` / `setTxBufferSize` large enough for one max framed stream message plus margin under normal poll latency (recommend **4096** each; at least above `kSerialMaxPayload` COBS worst-case).
2. `kSerialPlayCreditChunk` and the CLI host max chunk MUST satisfy `COBS(header + streamHdr + chunk) < RX buffer` with headroom (do not treat “credits below CDC buffer” as optional).
3. On `serialPlayFeed` failure: surface an error to the host (typed error/event or failed play path). Do not silently drop DATA and leave the host blocked on credit wait.
4. On feed/parse failure (or equivalent reject of an active session): **abort the play session** — release the global busy lock, clear session state, and restore the prior animation if PCM/playback had already started. Do not leave `active_` + busy set until a later `play_end` or reboot (otherwise later HTTP/serial play returns busy/503).
5. Host CLI: after a successful serial `play` open, wrap streaming in `try/finally` and best-effort send `play_end` while the port is still open when streaming fails before normal completion (cable yank / killed process still needs firmware abort; finally covers parser/credit errors).

**Alternatives:** Stop-and-wait per chunk (higher latency); unbounded push (OOM risk on C3); shrink chunks only and keep 256 B RX (works but poor throughput / fragile if LOG/TX contend).

### D4: Logging and boot noise

All application `serialLog*` paths emit LOG frames when `serial_log` is on; never raw text for those lines. Boot: no framed TX until settings loaded; optional SYNC beacon when protocol ready. Host discards until delimiter + successful decode (resync). HALT/panic: best-effort single raw line acceptable as last resort (document as non-framed).

**Alternatives:** Prefix-line mux (fragile with binary); dual CDC interfaces (not available on this product USB).

### D5: Connection mode / provisioning

NVS field e.g. `link_mode`: unset/empty = unprovisioned; `wifi` | `usb`. Setup Network step becomes “Link”: Wi‑Fi form **or** “USB only”. Provisioned = mode set (USB) or Wi‑Fi nets saved (wifi). Replace `Access::Configured` wifi-only gate with provisioned check. USB boot: skip STA + do not register HTTP routes (or never start WebServer). Factory reset clears mode + Wi‑Fi nets (existing reset semantics + mode).

Escape USB-only: factory reset only (product decision).

**`link_mode` validation split:** `settingsValidateLinkMode("")` (and NVS load) MUST still accept empty as the unprovisioned default. Settings **patches** MUST reject empty `link_mode`. When the device is already provisioned and **not** in setup AP mode, HTTP and serial settings paths MUST reject a changed `link_mode` (no silent `usb`↔`wifi` via `settings.set` / `POST /settings`). The setup AP Connection path remains the only place that sets mode during provisioning (USB-only save + teardown as below).

**End of Connection step (same session):** Completing either path MUST tear down the setup AP so provisioning is finished without waiting for reboot.

| Path | After successful save |
| --- | --- |
| Wi‑Fi | STA connected → reply → short delay → `wifiStopProvisioningAp()` → brief OLED status (IP, or “No IP” if empty) → idle eyes (`showIdleScreen` / `startEyes`) |
| USB-only | Reply 200 → short delay → `wifiStopProvisioningAp()` → clear `wifiProvisioningMode` → stop HTTP (no zombie config server) → brief OLED “USB-only” status (not “Join this WiFi”) → idle eyes → full serial API unlocked |

**OLED after leaving setup AP (same session):** Provisioning calls `stopEyes()`. Teardown MUST NOT leave a static status frame forever. Match cold-boot progress path: show status briefly (boot uses ~3s IP hold), then `showIdleScreen()` so eyes resume. Prefer one shared helper for boot + in-session leave. Wi‑Fi leave MAY also start welcome animation when `welcome` is enabled (same as boot); eyes idle is the minimum. Blocking `delay` in the HTTP handler is acceptable if it mirrors boot; a deferred non-blocking handoff is nicer but not required for correctness.

UI: drop mandatory “power-cycle before CLI works”; optional reboot remains fine for a clean boot. Power-cycle MUST NOT be required to leave the AP or use serial after USB-only save.

Do not leave `wifiProvisioningMode()==true` after USB-only save: serial gates on that flag and would stay ping-only until reboot.

### D6: CLI transport, project setup, and port cache

Two setup surfaces (do not conflate):

| Surface | Choice | Persist |
| --- | --- | --- |
| Device AP wizard (web UI) | `link_mode` wifi \| usb | NVS on robot |
| Project `tiny-engineer setup` | host transport http \| serial | project `.env` |

Host resolution: `--transport` > `TINY_ENGINEER_TRANSPORT` > default `http`.

Project setup flow:

1. Ask transport (`http` \| `serial`; default `http`)
2. If `http`: ask/merge URL (`TINY_ENGINEER_URL`) as today
3. If `serial`: optional port prompt; merge `TINY_ENGINEER_PORT` when set; do not require URL
4. Merge `TINY_ENGINEER_TRANSPORT` (same create/update/skip/force pattern as URL)
5. Non-interactive: `--transport`, `--force-transport`, optional `--port`; `--yes` defaults transport to `http` if unset
6. Uninstall `--purge-env` strips `TRANSPORT` and `PORT` with URL/token

Runtime:

- Serial: `--port` / `TINY_ENGINEER_PORT`; else autodetect (VID/PID + SYNC/ping); cache e.g. `~/.config/tiny-engineer/serial-port.json`; invalidate on open/probe fail
- New commands: `settings`, `test`, `servo`, `reset` (confirm / `--yes`), `monitor`
- Shared robot client for `anim` / `play` / `hook` / `doctor`
- Agent packages: shared client or CLI so hooks honor project `.env` transport

### D7: Contention

Global “busy” for play/long tests: second transport gets busy error. Short ops (anim, settings get) may still be serialized on device loop — document last-writer for anim if needed; play is exclusive.

### D8: Loop-task stack for serial pack/decode

`SerialFrameDecoder::onDelimiter_` holds a `decoded[kSerialMaxPayload]` (2048) while the frame handler runs; `sendPacked` adds `message[kSerialHeaderSize + kSerialMaxPayload]` (~2055); JSON responses add another ~1 KB. Nested on the Arduino-ESP32 `loop` task (~8 KB stack) this can reset the robot during serial play/LOG. Keep those large buffers **static or file-scope** (same pattern as `g_encodeBuf`). Poll/handler path is single-threaded on `loop`, so reentrancy is not required.

### D9: PlatformIO monitor

Primary: `tiny-engineer monitor`. Optional follow-up: PIO `monitor_encoding=latin1` + custom filter to print LOG text. Document exclusive port use. Dev escape hatch compile flag for raw ASCII logs only if needed during bring-up (not default firmware).

## Risks / Trade-offs

- **[Risk] Boot/ROM noise corrupts first session** → Mitigation: host resync + SYNC; device quiet until ready
- **[Risk] CDC RX overrun on stream DATA** → Observed on HW: play start OK, first ~1041 B DATA vs default HWCDC RX 256 → overflow → credit timeout. Mitigation: enlarge RX/TX before `begin` **and** keep credit chunk well below RX with margin; verify with CLI `play` of a short 22050 mono WAV on device
- **[Risk] Stuck play session after stream Error** → Feed/parse fail returns Error but leaves busy/session active; CLI closes port without `play_end`. Mitigation: firmware abort on feed fail (D3); CLI `finally` → `play_end` (D3); optional idle timeout later (out of P0–P1)
- **[Risk] Loop-task stack overflow during serial** → Nested decode + pack + JSON locals. Mitigation: D8 static/file-scope buffers
- **[Risk] Empty or mid-life `link_mode` patch** → Patch validator reused load-time empty-ok; provisioned `settings.set` could clear mode or switch `usb`↔`wifi` without factory reset. Mitigation: D5 patch gates
- **[Risk] PIO monitor unusable with COBS** → Mitigation: CLI monitor; docs; optional filter later
- **[Risk] Port exclusive (monitor vs CLI vs upload)** → Mitigation: document; clear CLI errors on busy port
- **[Risk] Hook latency from serial open/probe** → Mitigation: port cache; short timeouts; fail-open
- **[Risk] Dual transport races** → Mitigation: busy lock for play; simple serialization for writes
- **[Trade-off] Factory-reset-only escape** → Simpler security/UX story; slower recovery than in-band mode switch
- **[Trade-off] Default transport HTTP** → Backward compatible; corporate USB users pick serial once in `tiny-engineer setup` (writes `.env`); flags still override

## Migration Plan

1. Ship firmware with protocol + `link_mode`; existing devices without key behave as Wi‑Fi mode when nets exist (migration: if nets saved and mode unset → `wifi`).
2. Ship CLI with transport default HTTP; `setup` writes `TINY_ENGINEER_TRANSPORT` for new projects; serial opt-in via setup or env.
3. Docs: device Connection vs project setup; corporate USB-only path; monitor; factory reset.
4. Rollback: flash previous firmware; CLI without `TINY_ENGINEER_TRANSPORT=serial` keeps working against old HTTP-only devices (serial commands fail gracefully).

## Open Questions

- Exact USB VID/PID match list across macOS/Windows/Linux CDC product strings (resolve during implement with a spike on real hardware).
- Whether optional PIO LOG filter ships in the same change or a follow-up (default: follow-up unless cheap).
