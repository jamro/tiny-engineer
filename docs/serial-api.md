# Serial API

USB CDC transport for the same JSON control ops as the [HTTP API](api.md). Wi-Fi remains mandatory for a configured robot; serial is an **additional** always-on channel (trusted — no Bearer auth).

Source: [`src/serial/serial_api.cpp`](../src/serial/serial_api.cpp), shared dispatch in [`src/api/`](../src/api/). Baud **115200** (same as `monitor_speed` / `Serial.begin`).

The unified CLI can use this transport via `TINY_ENGINEER_SERIAL` or `--serial` — see [integration.md](integration.md).

## Framing

Newline-delimited UTF-8. One request or reply per line.

### Host → device

One JSON object per line (max **1024** bytes):

```json
{"v":1,"id":42,"op":"POST /anim","q":{"name":"thinking"}}
```

| Field | Rule |
| --- | --- |
| `v` | Protocol version; must be `1` |
| `id` | Unsigned integer; echoed in the reply |
| `op` | `"METHOD /path"` (single space), paths match HTTP |
| `q` | Object of **string** query params (same names/values as HTTP). Omit or `{}` if none |
| `len` | Required for `POST /play` / `POST /play/<name>` only: byte length of the WAV that follows (1…32 MiB) |

Most ops are JSON-only. **`POST /play`** is hybrid: after the begin line, the host sends exactly `len` raw WAV bytes (no base64), then waits for one `R` reply. On error the device still drains remaining bytes so the stream stays in sync. `serial_log` `L` lines are suppressed while those bytes are received.

**Body idle abort:** if no `/play` body byte arrives for **30 seconds** while `len` is still outstanding, the device ends the play session and sends one `R` reply with status **408** and `error` `play body incomplete` (or the earlier reject status if the session was already rejected). While declared bytes remain outstanding, the device stays in body-drain mode: further serial input is consumed as body bytes (discarded), not as line requests. A second 30-second idle with bytes still outstanding drops the remainder and returns to line mode. After an abort reply, the host must not send the rest of that body and must not send another request until that second idle returns the device to line mode. Reopening the host port does not clear drain mode on the device.

Oversized line, invalid JSON, or `v` ≠ 1 → reply with `status` **400** and no dispatch.

### Device → host

**API reply** (always, even when `serial_log` is off):

```text
R{"v":1,"id":42,"status":200,"body":{"ok":true,"animation":"thinking"}}
```

Literal prefix `R`, then one JSON object, then `\n`.

| Field | Meaning |
| --- | --- |
| `v` | `1` |
| `id` | Echo of the request id |
| `status` | Same codes as HTTP (`200`, `400`, `404`, `405`, `503`, …) |
| `body` | Same JSON object the HTTP route would return |

Serial never requires Bearer; it does not emit **401** for a missing token.

**Log lines** (only when settings `serial_log` is on):

```text
L WIFI SETUP
```

Prefix `L` + space + message + `\n`. Multi-part log prints share one `L ` at the start of the line. Host RPC clients should ignore every line that does not start with `R`.

## Supported ops

| `op` | Notes |
| --- | --- |
| `GET /auth` | Public on HTTP; same JSON here |
| `GET /health` | |
| `GET /anim` | Requires Wi-Fi credentials saved (else **503**) |
| `POST /anim` | Query `name`; requires Wi-Fi credentials saved |
| `GET /settings` | |
| `POST /settings` | Query params as HTTP |
| `POST /settings/reset` | |
| `POST /test/audio` | Requires Wi-Fi credentials saved |
| `POST /test/audio/bell` | Requires Wi-Fi credentials saved |
| `POST /test/screen` | Requires Wi-Fi credentials saved |
| `POST /test/movement` | Requires Wi-Fi credentials saved |
| `POST /test/led` | Requires Wi-Fi credentials saved |
| `POST /test/servo` | Query `index`, `angle`; requires Wi-Fi credentials saved |
| `POST /setup/servo` | Setup AP only (else **400**) |
| `POST /setup/led` | Setup AP only |
| `POST /setup/audio` | Setup AP only |
| `POST /setup/oled` | Setup AP only |
| `POST /servos/park` | No Wi-Fi gate; smooth mid + pose persist + hold PWM until reboot |
| `POST /play` | Begin JSON must include `len`; then raw WAV (same format as HTTP). Requires Wi-Fi credentials saved |
| `POST /play/<name>` | Same; `name` = `talking`\|`typing`\|`reading`\|`thinking`\|`none` |

Unknown `op` → **404** `{"ok":false,"error":"not found"}`. Wrong method for a known path → **405** `method not allowed`.

**Not on serial:** HTML / `/ui/*`, CORS, `PUT /settings/wifi` (JSON body). Use HTTP for those.

### `POST /play` framing

```text
{"v":1,"id":42,"op":"POST /play","len":123456}\n
<123456 bytes of WAV>
R{"v":1,"id":42,"status":200,"body":{"ok":true,"played_ms":3000}}\n
```

Incomplete body (host stops or stalls longer than the 30-second idle limit):

```text
R{"v":1,"id":42,"status":408,"body":{"ok":false,"error":"play body incomplete"}}\n
```

Pauses between body bytes are allowed up to that idle limit; there is no shorter undocumented timeout.

## Example

```bash
# Replace with your CDC device path (tiny-engineer ports)
printf '%s\n' '{"v":1,"id":1,"op":"GET /health","q":{}}' > /dev/cu.usbmodemXXXX
```

Or with the CLI:

```bash
TINY_ENGINEER_SERIAL=/dev/cu.usbmodemXXXX tiny-engineer anim thinking
tiny-engineer doctor --serial /dev/cu.usbmodemXXXX
tiny-engineer anim thinking --serial          # auto-detect (VID + health probe)
tiny-engineer play clip.wav --serial=auto
TINY_ENGINEER_SERIAL=auto tiny-engineer doctor
```

Auto-detect prefers Espressif USB devices (VID `303A`, manufacturer / Windows friendly name, PID `1001` USB-Serial/JTAG ranked first). Path norms: macOS `tty.*` → `cu.*` for open; Linux `/dev/ttyACM*`; Windows `COMn`. Candidates without a clear Espressif ID fall back to CDC-like paths only. Last good path (`~/.cache/tiny-engineer/serial-port.json`) is reused when still listed and `GET /health` succeeds (cleared when serial RPC is unreachable). Otherwise probes candidates in parallel with `GET /health` (~800 ms each) and requires exactly one match.

## Host tips

1. Open the port at 115200.
2. Write one request line ending in `\n`.
3. For `/play`, write exactly `len` WAV bytes next (no extra framing). Pace about realtime (~44 KB/s for 16-bit mono 22050 Hz). Much faster overflows CDC during `I2S.write`; much slower stretches speech with gaps. Do not pause longer than **30 seconds** between body bytes (device idle abort).
4. Read lines until an `R` line whose JSON `id` matches; ignore `L` and other noise.
5. Parse `status` and `body` like an HTTP response. On **408** `play body incomplete`, stop sending that body and wait for the second 30-second idle (device back in line mode) before the next request. Reopening the port does not clear body-drain mode.

Blocking test/setup/`/play` ops hold the device until motion/audio finishes (same as HTTP).
