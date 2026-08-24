# HTTP API

JSON HTTP server on the ESP32-C3. Source: [`src/http/http_server.cpp`](../src/http/http_server.cpp) and handlers under [`src/http/`](../src/http/), test routes in [`src/http/test_handlers.cpp`](../src/http/test_handlers.cpp).

Listens on **port 80** after STA Wi-Fi connects, or during setup AP mode at `http://192.168.4.1/`. Base URL is the board IP (OLED / serial) or `http://tiny-engineer.local` (mDNS, 2.4 GHz only).

Optional auth: when an `access_token` is configured in settings, all JSON API routes require `Authorization: Bearer <token>`. Empty token (default) means no auth. `GET /auth` is always public and reports whether auth is required. Missing/wrong token → **401** `{"ok":false,"error":"unauthorized"}`. HTML panel routes stay public (the UI prompts for the token). `Content-Type: application/json`. CORS: `Access-Control-Allow-Origin: *`, `Access-Control-Allow-Headers: Authorization`.

When WiFi credentials are not saved yet, control APIs (`/anim`, `/test/*`) return **503** `{"ok":false,"error":"wifi not configured"}`. Setup routes (`/`, `/config`, `/auth`, `/health`, `/settings`) stay available on the setup AP.

If boot WiFi credentials are missing, the device opens setup AP mode (`TinyEngineer-XXXX`) and serves the Config page. If saved credentials fail, it reopens setup AP mode. Hardware tests: [`docs/hardware/testing.md`](hardware/testing.md). How to wire this API into AI tools: [`integration.md`](integration.md).

Firmware answers mDNS A and AAAA (IPv6 link-local) so macOS does not wait 2–3s on a missing AAAA. If a client still pauses on the hostname, force IPv4 (`curl -4`). The IP on the OLED skips DNS entirely.

## Endpoints

### `GET /`

HTML endpoint index. Lists all routes plus supported parameters for `/anim`, `/settings`, and `/test/servo`. Safe, no hardware side effects. Always public (no Bearer required).

Open in a browser:

```bash
curl http://tiny-engineer.local/
```

Returns `Content-Type: text/html; charset=utf-8`.

### `GET /auth`

Whether API auth is required. Always public. Safe, no hardware side effects. Does not reveal the token.

```bash
curl http://tiny-engineer.local/auth
```

```json
{ "ok": true, "required": false, "wifi_configured": true, "provisioning": false }
```

| Field | Meaning |
| --- | --- |
| `ok` | Always `true` on this route |
| `required` | `true` when a non-empty access token is configured |
| `wifi_configured` | `true` when WiFi credentials are saved in NVS |
| `provisioning` | `true` when the setup AP is active |

### `GET /health`

Health. Safe, no hardware side effects. Uses live `WiFi.status()`, not the boot-time flag. Requires Bearer when auth is enabled.

```bash
curl http://tiny-engineer.local/health
curl -H "Authorization: Bearer YOUR_TOKEN" http://tiny-engineer.local/health
```

```json
{
  "ok": true,
  "uptime_ms": 12345,
  "free_heap": 120000,
  "heap_size": 320000,
  "cpu_temp_c": 41.2,
  "wifi": {
    "connected": true,
    "ip": "192.168.1.10",
    "rssi": -42,
    "hostname": "tiny-engineer.local"
  },
  "wifi_configured": true,
  "provisioning": false,
  "setup_ap_ssid": "",
  "setup_ap_ip": "",
  "oled": true
}
```

| Field | Meaning |
| --- | --- |
| `ok` | Always `true` on this route |
| `uptime_ms` | `millis()` since boot |
| `free_heap` | `ESP.getFreeHeap()` — bytes available for allocation |
| `heap_size` | `ESP.getHeapSize()` — total heap bytes; usage % = `(1 - free_heap / heap_size) × 100` |
| `cpu_temp_c` | ESP32-C3 on-die temperature in °C (not ambient); `null` if the sensor is unavailable |
| `wifi.connected` | `WiFi.status() == WL_CONNECTED` |
| `wifi.ip` | STA IPv4, or `""` if down |
| `wifi.rssi` | dBm, or `0` if down |
| `wifi.hostname` | mDNS name from settings (`{hostname}.local`, default `tiny-engineer.local`) |
| `wifi_configured` | `true` when WiFi credentials are saved in NVS |
| `provisioning` | `true` when setup AP mode is active |
| `setup_ap_ssid` | Setup AP SSID when `provisioning` is true, else `""` |
| `setup_ap_ip` | Setup AP IP when `provisioning` is true (usually `192.168.4.1`), else `""` |
| `oled` | SSD1306 probed and initialized |

### `GET /settings`

Persistent settings from NVS. Safe, no hardware side effects. Developer guide for adding settings: [`settings.md`](settings.md).

```bash
curl http://tiny-engineer.local/settings
```

```json
{
  "ok": true,
  "sleep_timeout": 10,
  "hostname": "tiny-engineer",
  "volume": 70,
  "welcome": true,
  "continuous_timeout": 5,
  "loading": "progress",
  "access_token_set": false,
  "wifi_configured": true,
  "wifi_ssid": "MyNetwork",
  "wifi_password_set": true
}
```

| Field | Meaning |
| --- | --- |
| `sleep_timeout` | Minutes of continuous animation `none` before OLED blanks (default **10**). Any non-idle animation keeps the device awake and restarts the idle clock when returning to `none`. Other APIs do not affect the timer. |
| `hostname` | DHCP/mDNS label without `.local` (default **`tiny-engineer`**) |
| `volume` | Speaker gain percent for tones and WAV playback (default **70**) |
| `welcome` | Play welcome animation on boot when Wi-Fi connects (default **true**). Also enables head/neck motion during `sleep_inertia` loading |
| `continuous_timeout` | Minutes a continuous animation (`typing` / `reading` / `thinking`) may run before the firmware switches to `attention` then idle (default **5**) |
| `loading` | Boot loading screen: `progress` (bar + large IP for 3 s) or `sleep_inertia` (eyes wake; no bar/IP). Default **`progress`**. Applies on **next reboot** |
| `access_token_set` | `true` when a non-empty access token is stored (secret itself is never returned) |
| `wifi_configured` | `true` when a WiFi SSID is saved |
| `wifi_ssid` | Saved network name (password is never returned) |
| `wifi_password_set` | `true` when a non-empty WiFi password is saved |

### `POST /settings`

Update one or more settings. Query params. Values are written to NVS. `sleep_timeout`, `volume`, `welcome`, `continuous_timeout`, and `access_token` apply immediately; a changed `hostname` or `loading` takes effect on the **next reboot**. WiFi credentials (`wifi_ssid` + `wifi_password`) are accepted **only in setup AP mode**; they are **tested before save**; on success the device connects to the home network and returns `wifi_connect_success`, `wifi_ip`, and `wifi_hostname`. Outside setup AP → **400** `wifi setup only in AP mode`. Requires Bearer when auth is enabled.

```bash
curl -X POST "http://tiny-engineer.local/settings?sleep_timeout=2"
curl -X POST "http://tiny-engineer.local/settings?hostname=desk-bot"
curl -X POST "http://tiny-engineer.local/settings?volume=40"
curl -X POST "http://tiny-engineer.local/settings?welcome=0"
curl -X POST "http://tiny-engineer.local/settings?continuous_timeout=10"
curl -X POST "http://tiny-engineer.local/settings?loading=sleep_inertia"
curl -X POST "http://tiny-engineer.local/settings?access_token=secret"
curl -X POST "http://tiny-engineer.local/settings?access_token="
curl -X POST "http://192.168.4.1/settings?wifi_ssid=MyNetwork&wifi_password=secret"
curl -X POST "http://tiny-engineer.local/settings?sleep_timeout=10&hostname=tiny-engineer&volume=70&welcome=1&continuous_timeout=5&loading=progress"
```

```json
{
  "ok": true,
  "sleep_timeout": 1,
  "hostname": "desk-bot",
  "volume": 70,
  "welcome": true,
  "continuous_timeout": 5,
  "loading": "progress",
  "access_token_set": true,
  "wifi_configured": true,
  "wifi_ssid": "MyNetwork",
  "wifi_password_set": true,
  "wifi_connect_success": true,
  "wifi_ip": "192.168.1.10",
  "wifi_hostname": "tiny-engineer.local",
  "reboot_required": true
}
```

| Param | Type | Range |
| --- | --- | --- |
| `sleep_timeout` | integer | 1–1440 minutes (positive) |
| `hostname` | string | 1–31 chars, `[A-Za-z0-9-]`, not starting/ending with `-` |
| `volume` | integer | 0–100 percent |
| `welcome` | integer | `0` or `1` (boot welcome animation) |
| `continuous_timeout` | integer | 1–1440 minutes (positive) |
| `loading` | string | `progress` or `sleep_inertia` |
| `access_token` | string | 0–64 printable ASCII; empty string clears the token and disables auth |
| `wifi_ssid` | string | 1–32 chars; setup AP only; must be sent with `wifi_password` |
| `wifi_password` | string | 0–63 chars; setup AP only; empty allowed for open networks; tested before save |

At least one param required. Invalid or missing-all → **400**. WiFi params outside setup AP → **400** `wifi setup only in AP mode`. WiFi test failure → **400** with error such as `SSID not found`, `Auth failed`, or `Timeout`. `reboot_required` is present and `true` only when the saved hostname differs from the one used at this boot. After successful WiFi save, `wifi_connect_success`, `wifi_ip`, and `wifi_hostname` are included.

### `POST /settings/reset`

Factory reset: clears the NVS `te` namespace and restores all settings to compile-time defaults, including WiFi credentials. After reset, power-cycle (or press reset) so the device reopens setup AP mode and WiFi can be configured again. Until reboot, credentials are cleared in NVS but the radio may still be on the previous STA network. No query params. Requires Bearer when auth is enabled. Same JSON shape as `GET /settings`; includes `reboot_required` when the boot hostname, loading screen, or saved WiFi differed from defaults before reset.

The Config web UI clears the browser-stored Bearer token and shows a one-shot gate: power-cycle the device, then join the robot WiFi shown on the OLED and open the setup page. It does not poll for reconnect.

```bash
curl -X POST "http://tiny-engineer.local/settings/reset"
```

```json
{
  "ok": true,
  "sleep_timeout": 10,
  "hostname": "tiny-engineer",
  "volume": 70,
  "welcome": true,
  "continuous_timeout": 5,
  "loading": "progress",
  "access_token_set": false,
  "reboot_required": true
}
```

NVS write failure → **400** `{"ok":false,"error":"factory reset failed"}`.

### `POST /test/audio`

Plays 500 / 700 / 1000 Hz on the MAX98357A (`runSoundTest()`).

```bash
curl -X POST http://tiny-engineer.local/test/audio
```

```json
{ "ok": true, "test": "audio" }
```

### `POST /test/audio/bell`

Plays `assets/bell.wav` (44100 Hz mono PCM) from LittleFS (`playBell()`). First flash or after changing `data/bell.wav`, upload the filesystem:

```bash
pio run -t uploadfs
```

```bash
curl -X POST http://tiny-engineer.local/test/audio/bell
```

```json
{ "ok": true, "test": "bell" }
```

If the WAV is missing or unreadable, returns **500** with `{"ok":false,"error":"bell playback failed"}`.

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

Onboard WS2812 on GPIO10: R → G → B → white → off (`runRgbTest()`), then fades back to the current animation color (see [RGB LED](#rgb-led)).

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
| `animation` | `none`, `typing`, `reading`, `thinking`, `ring`, `welcome`, `attention`, `error`, or `abort` |

### `POST /anim`

Request an animation switch. Each animation runs at least **1s**; if the current one is shorter than that, the switch waits until the hold ends. Rapid posts during the hold keep only the **latest** request. Response `animation` is the currently playing name (may still be the old one until the hold ends). Hand moves use speed-limited servo wrappers.

| Param | Type | Values |
| --- | --- | --- |
| `name` | string | `none`, `typing`, `reading`, `thinking`, `ring`, `welcome`, `attention`, `error`, `abort` |

```bash
curl -X POST "http://tiny-engineer.local/anim?name=typing"
curl -X POST "http://tiny-engineer.local/anim?name=reading"
curl -X POST "http://tiny-engineer.local/anim?name=thinking"
curl -X POST "http://tiny-engineer.local/anim?name=ring"
curl -X POST "http://tiny-engineer.local/anim?name=welcome"
curl -X POST "http://tiny-engineer.local/anim?name=attention"
curl -X POST "http://tiny-engineer.local/anim?name=error"
curl -X POST "http://tiny-engineer.local/anim?name=abort"
curl -X POST "http://tiny-engineer.local/anim?name=none"
```

```json
{ "ok": true, "animation": "typing" }
```

| `name` | Behavior |
| --- | --- |
| `none` | Head/neck/body → mid; hands down (right `min`, left `max` — inverted scales), then hold |
| `typing` | **Continuous.** Alternating hands with randomness (15° band from hand limits). Head nods slowly on lowest 10° of head limits. Body sways ±5° around mid; neck counters opposite so head stays put. Runs until replaced, or until `continuous_timeout` triggers `attention`. Same-name re-`POST` refreshes that timeout without restarting motion. |
| `reading` | **Continuous.** Hands/body park as in `none`. Head nods on same lowest 10° band as typing, but slower. Neck sweeps ±10° around mid; right (angle down) slower than left (angle up). Occasional right-hand down-arrow bursts (1–3 presses, same 15° band as typing). Runs until replaced, or until `continuous_timeout` triggers `attention`. Same-name re-`POST` refreshes that timeout without restarting motion. |
| `thinking` | **Continuous.** Hands/body park as in `none`. Head (pitch) and neck (yaw) ease from the current pose into thinking poses (up + slight left/right). Move → pause → optional micro-adjust (sometimes chained) → pause; nearby pose drift with occasional larger shifts after ~2.2 s, more often over time. Axes stagger start/duration; no periodic sway. Runs until replaced, or until `continuous_timeout` triggers `attention`. Same-name re-`POST` refreshes that timeout without restarting motion. |
| `ring` | **One-shot** service-bell gesture; does not loop. Wind-up (body `min`, neck mid, head mid+10°, both hands `max`) → fast right-hand strike to `min+5°` with head to `min` → plays `bell.wav` once on strike (LittleFS; same `uploadfs` requirement as `/test/audio/bell`) → slower bounce to `min+20°` → return to `none` pose and stop. After completion, `GET /anim` reports `none`. |
| `welcome` | **One-shot** hello gesture synced to `welcome.wav` (~2.7 s). Right hand raises during "Hello, human.", holds through the pause, wiggles during "What are we building today?", then lowers. Head nods to mid+10° and returns. Plays automatically after successful Wi-Fi connect at boot; also via API. Requires `welcome.wav` on LittleFS (same `uploadfs` flow as `bell.wav`). After completion, `GET /anim` reports `none`. |
| `attention` | Friendly input-request gesture synced to `attention.wav` (~3.0 s, "pst... human.... you might want to take a look"). Moves into a calm prompt pose first (centered body/neck, head slightly up, right hand raised partway), waits until all servos stop, then plays audio with phased eye cues and light neck/head/hand motion during playback (whisper hold → lean toward user → glance/point on "take a look"). After audio ends (or if audio fails to start), holds a gentle waiting loop for **1 minute** (soft head/neck drifts plus occasional slight right-hand waves), then returns to `none`. Requires `attention.wav` on LittleFS (same `uploadfs` flow as `bell.wav`). |
| `error` | Critical task-obstacle gesture synced to `error.wav` (~2.2 s, "Uh-oh. Human, we have a problem."). Moves into an obstacle-presenting pose first (body/neck angled toward the task, head concerned/down, right hand presenting the blocker, left hand indicating task area), waits until the pose settles, then plays audio with small nervous head/neck glances during playback. After audio ends (or if audio fails to start), holds a subtle blocked loop for **1 minute**, then returns to `none`. Requires `error.wav` on LittleFS (same `uploadfs` flow as `bell.wav`). |
| `abort` | **One-shot** resigned abort gesture synced to `abort.wav` (~2.5 s, "Fine. I didn't want to finish it anyway."). Raises both hands, lifts the head, and twists the neck sideways before audio starts. During playback it shrugs, dips the head, and adds a dismissive side twist with matching eye glances/squints. Requires `abort.wav` on LittleFS (same `uploadfs` flow as `bell.wav`). After completion, returns to `none` pose and `GET /anim` reports `none`. |

Wrong params return **400**:

| `error` | When |
| --- | --- |
| `missing name` | Query param `name` absent |
| `unknown animation` | `name` not `none`, `typing`, `reading`, `thinking`, `ring`, `welcome`, `attention`, `error`, or `abort` |

## Errors

| Status | Body | When |
| --- | --- | --- |
| `400` | `{"ok":false,"error":"..."}` | Bad `/test/servo` or `/anim` params (see tables above) |
| `401` | `{"ok":false,"error":"unauthorized"}` | Access token configured and `Authorization: Bearer` missing or wrong |
| `404` | `{"ok":false,"error":"not found"}` | Unknown path |
| `405` | `{"ok":false,"error":"method not allowed"}` | Wrong method on a `/test/*`, `/settings`, `/settings/reset`, `/anim`, or `/auth` path |

Test routes are **POST**. GET/prefetch would move hardware. `/anim` allows **GET** (read) and **POST** (set). `/auth` is **GET** only and always public.

## Behaviour

Test handlers **block** until the test finishes. The client waits. After each test, OLED returns to `ROBOT READY` plus IP (or `WIFI FAIL`).

`/anim` responses return immediately; typing motion runs in the main loop via non-blocking servo updates. Animation switches may defer up to 1s so the active animation holds its minimum duration; only the latest pending request is applied. Continuous animations (`typing`, `reading`, `thinking`) that stay active longer than `continuous_timeout` minutes switch to `attention`, which holds ~1 minute then finishes to `none`. Re-`POST`ing the same continuous animation (e.g. `typing` while already `typing`) does not restart motion, but resets the continuous-timeout clock.

The onboard RGB LED follows the active animation (see [RGB LED](#rgb-led)).

One request at a time — the Arduino `WebServer` is single-threaded.

## RGB LED

Onboard WS2812 on **GPIO10** (`RGB_LED_PIN`). Animation-driven colors are handled in [`src/hardware/rgb.cpp`](../src/hardware/rgb.cpp) and switch when `POST /anim` applies a new state (same 1s minimum hold as servos/eyes).

| Animation | LED color |
| --- | --- |
| `typing`, `reading`, `thinking`, `welcome`, `ring` | White (full intensity) |
| `attention`, `error` | Red pulse: 500 ms 10%→100%, 500 ms hold 100%, 500 ms 100%→10%, repeat |
| `abort` | Red (full intensity, solid) |
| `none` | Off |

Transitions take **1 s** with smooth fade in/out (pulse modes start immediately, no enter fade):

- **Off ↔ color** — single 1 s fade (e.g. idle → typing fades in white; ring → `none` fades out).
- **White ↔ solid red** — fade out to black (500 ms), then fade in to the new color (500 ms).
- **Leaving pulse** — 1 s fade from the current pulse brightness to the new target.

Switching between animations that share the same color (e.g. `typing` → `reading`) does not restart a fade.

Boot uses dim green `(0, 32, 0)` as a status indicator during init. After `ROBOT READY`, the LED fades to white if `welcome` runs (Wi-Fi OK) or off if idle. Fatal PCA9685 / I2S errors set solid dim red and hang — not animation-driven.

`POST /test/led` runs a hardware colour cycle and then restores the current animation LED state.
