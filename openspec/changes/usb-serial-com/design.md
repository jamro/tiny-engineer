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

Protocol version starts at `1`. Max payload capped (recommend 1024 bytes for stream chunks; ≤2048 for JSON) to bound decode buffers.

**COBS library:** vendor [nanocobs](https://github.com/charlesnicholson/nanocobs) (`cobs.c`/`cobs.h`) — C99, no libc, incremental APIs, ~1–1.5 KB flash. Host: npm `nanocobs` or same algorithm in JS for byte identity.

**Alternatives:** PatrickBaus (254-byte only — reject); ESPP COBS (`std::vector` — reject); SLIP (similar, less tooling match).

### D3: Stream flow control

Play opens a stream; device advertises credit window (2–4 × 512–1024 B). Host sends stream frames only while credits > 0; device returns credits as I2S drains (aligned with existing `writeMonoToSpeaker` blocking). Reuse `WavStreamParser` sink path from HTTP play.

**Alternatives:** Stop-and-wait per chunk (higher latency); unbounded push (OOM risk on C3).

### D4: Logging and boot noise

All application `serialLog*` paths emit LOG frames when `serial_log` is on; never raw text for those lines. Boot: no framed TX until settings loaded; optional SYNC beacon when protocol ready. Host discards until delimiter + successful decode (resync). HALT/panic: best-effort single raw line acceptable as last resort (document as non-framed).

**Alternatives:** Prefix-line mux (fragile with binary); dual CDC interfaces (not available on this product USB).

### D5: Connection mode / provisioning

NVS field e.g. `link_mode`: unset/empty = unprovisioned; `wifi` | `usb`. Setup Network step becomes “Link”: Wi‑Fi form **or** “USB only”. Provisioned = mode set (USB) or Wi‑Fi nets saved (wifi). Replace `Access::Configured` wifi-only gate with provisioned check. USB boot: skip STA + do not register HTTP routes (or never start WebServer). Factory reset clears mode + Wi‑Fi nets (existing reset semantics + mode).

Escape USB-only: factory reset only (product decision).

### D6: CLI transport and port cache

- `TINY_ENGINEER_TRANSPORT` / `--transport` ∈ {`http`,`serial`}; **default `http`**
- Serial: `--port` / `TINY_ENGINEER_PORT`; else autodetect (VID/PID + SYNC/ping probe); cache file e.g. `~/.config/tiny-engineer/serial-port.json` (port path, probe id, timestamp); invalidate on open/probe fail
- New commands: `settings`, `test`, `servo`, `reset` (confirm / `--yes`), `monitor`
- Shared robot client interface used by `anim`/`play`/`hook`/`doctor`
- Agent packages: prefer routing through CLI or shared client module so hooks honor transport

### D7: Contention

Global “busy” for play/long tests: second transport gets busy error. Short ops (anim, settings get) may still be serialized on device loop — document last-writer for anim if needed; play is exclusive.

### D8: PlatformIO monitor

Primary: `tiny-engineer monitor`. Optional follow-up: PIO `monitor_encoding=latin1` + custom filter to print LOG text. Document exclusive port use. Dev escape hatch compile flag for raw ASCII logs only if needed during bring-up (not default firmware).

## Risks / Trade-offs

- **[Risk] Boot/ROM noise corrupts first session** → Mitigation: host resync + SYNC; device quiet until ready
- **[Risk] CDC RX overrun without credits** → Mitigation: mandatory credit window sized below CDC buffer; spike measure on HW
- **[Risk] PIO monitor unusable with COBS** → Mitigation: CLI monitor; docs; optional filter later
- **[Risk] Port exclusive (monitor vs CLI vs upload)** → Mitigation: document; clear CLI errors on busy port
- **[Risk] Hook latency from serial open/probe** → Mitigation: port cache; short timeouts; fail-open
- **[Risk] Dual transport races** → Mitigation: busy lock for play; simple serialization for writes
- **[Trade-off] Factory-reset-only escape** → Simpler security/UX story; slower recovery than in-band mode switch
- **[Trade-off] Default transport HTTP** → Backward compatible; corporate USB users must set env once

## Migration Plan

1. Ship firmware with protocol + `link_mode`; existing devices without key behave as Wi‑Fi mode when nets exist (migration: if nets saved and mode unset → `wifi`).
2. Ship CLI with transport default HTTP; serial opt-in.
3. Docs: corporate USB-only setup path; monitor; factory reset.
4. Rollback: flash previous firmware; CLI without `TINY_ENGINEER_TRANSPORT=serial` keeps working against old HTTP-only devices (serial commands fail gracefully).

## Open Questions

- Exact USB VID/PID match list across macOS/Windows/Linux CDC product strings (resolve during implement with a spike on real hardware).
- Whether optional PIO LOG filter ships in the same change or a follow-up (default: follow-up unless cheap).
