# HTTP API

JSON HTTP server on the ESP32-C3. Source: [`src/http_server.cpp`](../src/http_server.cpp).

Listens on **port 80** after STA Wi-Fi connects. Base URL is the board IP (OLED / serial) or `http://tiny-engineer.local` (mDNS, 2.4 GHz only).

No auth. `Content-Type: application/json`. `Access-Control-Allow-Origin: *`.

If boot Wi-Fi fails, the server does not start (`HTTP: skipped (no wifi)`). Hardware tests: [`docs/hardware/testing.md`](hardware/testing.md).

Firmware answers mDNS A and AAAA (IPv6 link-local) so macOS does not wait 2–3s on a missing AAAA. If a client still pauses on the hostname, force IPv4 (`curl -4`). The IP on the OLED skips DNS entirely.

## Endpoints

### `GET /`

Health. Safe, no hardware side effects. Uses live `WiFi.status()`, not the boot-time flag.

```bash
curl http://tiny-engineer.local/
```

```json
{
  "ok": true,
  "uptime_ms": 12345,
  "free_heap": 120000,
  "wifi": {
    "connected": true,
    "ip": "192.168.1.10",
    "rssi": -42,
    "hostname": "tiny-engineer.local"
  },
  "oled": true
}
```

| Field | Meaning |
| --- | --- |
| `ok` | Always `true` on this route |
| `uptime_ms` | `millis()` since boot |
| `free_heap` | `ESP.getFreeHeap()` |
| `wifi.connected` | `WiFi.status() == WL_CONNECTED` |
| `wifi.ip` | STA IPv4, or `""` if down |
| `wifi.rssi` | dBm, or `0` if down |
| `wifi.hostname` | Always `tiny-engineer.local` |
| `oled` | SSD1306 probed and initialized |

### `POST /test/audio`

Plays 500 / 700 / 1000 Hz on the MAX98357A (`runSoundTest()`).

```bash
curl -X POST http://tiny-engineer.local/test/audio
```

```json
{ "ok": true, "test": "audio" }
```

### `POST /test/screen`

OLED demo: title, `HELLO`, X in a box (`runOledTest()`). No-op if the panel is missing.

```bash
curl -X POST http://tiny-engineer.local/test/screen
```

```json
{ "ok": true, "test": "screen" }
```

### `POST /test/movement`

All five servos: 90° → 105° → 75° → 90° (`runServoTest()`). Needs a strong 5 V supply — see [power.md](hardware/power.md).

```bash
curl -X POST http://tiny-engineer.local/test/movement
```

```json
{ "ok": true, "test": "movement" }
```

### `POST /test/led`

Onboard WS2812 on GPIO10: R → G → B → white → off (`runRgbTest()`), then back to green ready.

```bash
curl -X POST http://tiny-engineer.local/test/led
```

```json
{ "ok": true, "test": "led" }
```

### `POST /test/servo`

Smoothly move one servo to an angle at **~40°/s** (`SERVO_SPEED_DEG_S`) from its last commanded position. Query params required. Handler blocks until the move finishes.

| Param | Type | Range |
| --- | --- | --- |
| `index` | integer | `0`–`4` |
| `angle` | number | `0`–`180` |

```bash
curl -X POST "http://tiny-engineer.local/test/servo?index=0&angle=90"
```

```json
{ "ok": true, "test": "servo", "index": 0, "angle": 90 }
```

Wrong params return **400** and do not move any servo:

| `error` | When |
| --- | --- |
| `missing index or angle` | Either query param absent |
| `invalid index` | Non-integer `index` (e.g. `abc`, `1x`) |
| `index out of range` | `index` outside `0`–`4` |
| `invalid angle` | Non-numeric `angle` |
| `angle out of range` | `angle` outside `0`–`180` |

Assembled robot: prefer the safe band in [servos.md](hardware/servos.md); this route allows full electrical travel for bench bring-up.

### `GET /anim`

Current animation name. Default at boot: `none`. No hardware side effects.

```bash
curl http://tiny-engineer.local/anim
```

```json
{ "ok": true, "animation": "none" }
```

| Field | Meaning |
| --- | --- |
| `animation` | `none` or `typing` |

### `POST /anim`

Switch animation immediately. Current motion stops; robot transitions to the new animation. Hand moves use speed-limited servo wrappers.

| Param | Type | Values |
| --- | --- | --- |
| `name` | string | `none`, `typing` |

```bash
curl -X POST "http://tiny-engineer.local/anim?name=typing"
curl -X POST "http://tiny-engineer.local/anim?name=none"
```

```json
{ "ok": true, "animation": "typing" }
```

| `name` | Behavior |
| --- | --- |
| `none` | Hands freeze; no motion |
| `typing` | Alternating hands (one at a time): right 40°↔50°, left 130°↔140° (inverted scales) |

Wrong params return **400**:

| `error` | When |
| --- | --- |
| `missing name` | Query param `name` absent |
| `unknown animation` | `name` not `none` or `typing` |

## Errors

| Status | Body | When |
| --- | --- | --- |
| `400` | `{"ok":false,"error":"..."}` | Bad `/test/servo` or `/anim` params (see tables above) |
| `404` | `{"ok":false,"error":"not found"}` | Unknown path |
| `405` | `{"ok":false,"error":"method not allowed"}` | Wrong method on a `/test/*` or `/anim` path |

Test routes are **POST**. GET/prefetch would move hardware. `/anim` allows **GET** (read) and **POST** (set).

## Behaviour

Test handlers **block** until the test finishes. The client waits. After each test, OLED returns to `ROBOT READY` plus IP (or `WIFI FAIL`).

`/anim` responses return immediately; typing motion runs in the main loop via non-blocking servo updates.

One request at a time — the Arduino `WebServer` is single-threaded.
