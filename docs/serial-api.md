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

No request body in this protocol version (no `/play` WAV upload over serial).

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

Unknown `op` → **404** `{"ok":false,"error":"not found"}`. Wrong method for a known path → **405** `method not allowed`.

**Not on serial:** HTML / `/ui/*`, CORS, `PUT /settings/wifi` (JSON body), `POST /play` (binary WAV). Use HTTP for those. WAV-over-serial is a possible later extension (length-prefixed binary after a JSON begin line).

## Example

```bash
# Replace with your CDC device path (tiny-engineer ports)
printf '%s\n' '{"v":1,"id":1,"op":"GET /health","q":{}}' > /dev/cu.usbmodemXXXX
```

Or with the CLI:

```bash
TINY_ENGINEER_SERIAL=/dev/cu.usbmodemXXXX tiny-engineer anim thinking
tiny-engineer doctor --serial /dev/cu.usbmodemXXXX
```

## Host tips

1. Open the port at 115200.
2. Write one request line ending in `\n`.
3. Read lines until an `R` line whose JSON `id` matches; ignore `L` and other noise.
4. Parse `status` and `body` like an HTTP response.

Blocking test/setup ops hold the device until motion/audio finishes (same as HTTP).
