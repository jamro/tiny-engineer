# HTTP API

JSON HTTP server on the ESP32-C3. Source: [`src/http_server.cpp`](../src/http_server.cpp).

Listens on **port 80** after STA Wi-Fi connects. Base URL is the board IP (OLED / serial) or `http://tiny-engineer.local` (mDNS, 2.4 GHz only).

No auth. `Content-Type: application/json`. `Access-Control-Allow-Origin: *`.

If boot Wi-Fi fails, the server does not start (`HTTP: skipped (no wifi)`). Hardware tests: [`docs/hardware/testing.md`](hardware/testing.md).

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

## Errors

| Status | Body | When |
| --- | --- | --- |
| `404` | `{"ok":false,"error":"not found"}` | Unknown path |
| `405` | `{"ok":false,"error":"method not allowed"}` | Wrong method on a `/test/*` path (GET instead of POST) |

Test routes are **POST**. GET/prefetch would move hardware.

## Behaviour

Handlers **block** until the test finishes. The client waits. After each test, OLED returns to `ROBOT READY` plus IP (or `WIFI FAIL`).

One request at a time — the Arduino `WebServer` is single-threaded.
