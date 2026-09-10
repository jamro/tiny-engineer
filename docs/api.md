# HTTP API

JSON HTTP server on the ESP32-C3. Source: [`src/http/http_server.cpp`](../src/http/http_server.cpp) and handlers under [`src/http/`](../src/http/), test routes in [`src/http/test_handlers.cpp`](../src/http/test_handlers.cpp).

Listens on **port 80** after STA Wi-Fi connects, or during setup AP mode at `http://192.168.4.1/`. Base URL is the board IP (OLED / serial) or `http://tiny-engineer.local` (mDNS, 2.4 GHz only).

Optional auth: when an `access_token` is configured in settings, all JSON API routes require `Authorization: Bearer <token>`. Empty token (default) means no auth. `GET /auth` is always public and reports whether auth is required. Missing/wrong token → **401** `{"ok":false,"error":"unauthorized"}`. HTML panel routes stay public (the UI prompts for the token). `Content-Type: application/json`. CORS: `Access-Control-Allow-Origin: *`, `Access-Control-Allow-Headers: Authorization`.

When WiFi credentials are not saved yet, control APIs (`/anim`, `/test/*`) return **503** `{"ok":false,"error":"wifi not configured"}`. Setup routes (`/`, `/config`, `/auth`, `/health`, `/settings`, `/setup/servo`, `/setup/led`, `/setup/audio`, `/setup/oled`) stay available on the setup AP.

If boot WiFi credentials are missing, the device opens setup AP mode (`TinyEngineer-XXXX`) and serves a five-step setup wizard at `/config` (servo calibration, OLED orientation, RGB LED, speaker test, then Wi-Fi). If saved credentials fail, it reopens setup AP mode. Hardware tests: [`docs/hardware/testing.md`](hardware/testing.md). How to wire this API into AI tools: [`integration.md`](integration.md).

Firmware answers mDNS A and AAAA (IPv6 link-local) so macOS does not wait 2–3s on a missing AAAA. If a client still pauses on the hostname, force IPv4 (`curl -4`). The IP on the OLED skips DNS entirely.

## Endpoints

### `GET /`

HTML endpoint index. Lists all routes plus supported parameters for `/anim`, `/settings`, `/test/servo`, `/setup/servo`, `/setup/led`, `/setup/audio`, and `/setup/oled`. Safe, no hardware side effects. Always public (no Bearer required).

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
  "serial_log": false,
  "continuous_timeout": 5,
  "loading": "progress",
  "access_token_set": false,
  "wifi_configured": true,
  "wifi_ssid": "MyNetwork",
  "wifi_password_set": true,
  "servo_mins": [60, 40, 45, 35, 40],
  "servo_maxs": [130, 130, 135, 125, 130],
  "rgb_order": "GRB",
  "oled_rotate_180": false
}
```

| Field | Meaning |
| --- | --- |
| `sleep_timeout` | Minutes of continuous animation `none` before OLED blanks (default **10**). Any non-idle animation keeps the device awake and restarts the idle clock when returning to `none`. Other APIs do not affect the timer. |
| `hostname` | DHCP/mDNS label without `.local` (default **`tiny-engineer`**) |
| `volume` | Speaker gain percent for tones and WAV playback (default **70**) |
| `welcome` | Play welcome animation on boot when Wi-Fi connects (default **true**). Also enables head/neck motion during `sleep_inertia` loading |
| `serial_log` | USB serial debug logging (default **false**). When off, diagnostic `Serial` output is suppressed |
| `continuous_timeout` | Minutes a continuous animation (`typing` / `reading` / `thinking`) may run before the firmware switches to `attention` then idle (default **5**) |
| `loading` | Boot loading screen: `progress` (bar + large IP for 3 s) or `sleep_inertia` (eyes wake; no bar/IP). Default **`progress`**. Applies on **next reboot** |
| `access_token_set` | `true` when a non-empty access token is stored (secret itself is never returned) |
| `wifi_configured` | `true` when a WiFi SSID is saved |
| `wifi_ssid` | Saved network name (password is never returned) |
| `wifi_password_set` | `true` when a non-empty WiFi password is saved |
| `servo_mins` | Per-joint safe minimum degrees `[head, neck, left, right, body]` (stock defaults **60, 40, 45, 35, 40**) |
| `servo_maxs` | Per-joint safe maximum degrees (stock defaults **130, 130, 135, 125, 130**). Each max must be greater than the matching min |
| `rgb_order` | Onboard WS2812 wire-byte order (`RGB`, `RBG`, `GRB`, `GBR`, `BRG`, `BGR`). Default **`GRB`**. Survives factory reset |
| `oled_rotate_180` | OLED 180° rotation (`true` → `setRotation(2)`). Default **`false`**. Setup AP only. Survives factory reset |

### `POST /settings`

Update one or more settings. Query params. Values are written to NVS. `sleep_timeout`, `volume`, `welcome`, `serial_log`, `continuous_timeout`, and `access_token` apply immediately; a changed `hostname` or `loading` takes effect on the **next reboot**. WiFi credentials (`wifi_ssid` + `wifi_password`) are accepted **only in setup AP mode**; they are **tested before save**; on success the device connects to the home network and returns `wifi_connect_success`, `wifi_ip`, and `wifi_hostname`. `hostname` may be sent with those WiFi params (same validation as Config); it is saved before the STA connect so mDNS uses the chosen name immediately. Outside setup AP → **400** `wifi setup only in AP mode`. Servo ranges (`servo_mins` + `servo_maxs`) are also **setup AP only**; they apply immediately to motion clamps. Outside setup AP → **400** `servo setup only in AP mode`. RGB LED mapping (`rgb_order`) is **setup AP only** and applies immediately to the onboard LED. Outside setup AP → **400** `rgb setup only in AP mode`. OLED rotation (`oled_rotate_180`) is **setup AP only** and applies immediately to the display. Outside setup AP → **400** `oled setup only in AP mode`. Requires Bearer when auth is enabled.

```bash
curl -X POST "http://tiny-engineer.local/settings?sleep_timeout=2"
curl -X POST "http://tiny-engineer.local/settings?hostname=desk-bot"
curl -X POST "http://tiny-engineer.local/settings?volume=40"
curl -X POST "http://tiny-engineer.local/settings?welcome=0"
curl -X POST "http://tiny-engineer.local/settings?serial_log=1"
curl -X POST "http://tiny-engineer.local/settings?continuous_timeout=10"
curl -X POST "http://tiny-engineer.local/settings?loading=sleep_inertia"
curl -X POST "http://tiny-engineer.local/settings?access_token=secret"
curl -X POST "http://tiny-engineer.local/settings?access_token="
curl -X POST "http://192.168.4.1/settings?wifi_ssid=MyNetwork&wifi_password=secret&hostname=desk-bot"
curl -X POST "http://192.168.4.1/settings?servo_mins=60,40,45,35,40&servo_maxs=130,130,135,125,130"
curl -X POST "http://192.168.4.1/settings?rgb_order=GRB"
curl -X POST "http://192.168.4.1/settings?oled_rotate_180=1"
curl -X POST "http://tiny-engineer.local/settings?sleep_timeout=10&hostname=tiny-engineer&volume=70&welcome=1&serial_log=0&continuous_timeout=5&loading=progress"
```

```json
{
  "ok": true,
  "sleep_timeout": 1,
  "hostname": "desk-bot",
  "volume": 70,
  "welcome": true,
  "serial_log": false,
  "continuous_timeout": 5,
  "loading": "progress",
  "access_token_set": true,
  "wifi_configured": true,
  "wifi_ssid": "MyNetwork",
  "wifi_password_set": true,
  "servo_mins": [60, 40, 45, 35, 40],
  "servo_maxs": [130, 130, 135, 125, 130],
  "rgb_order": "GRB",
  "oled_rotate_180": false,
  "wifi_connect_success": true,
  "wifi_ip": "192.168.1.10",
  "wifi_hostname": "desk-bot.local",
  "reboot_required": true
}
```

| Param | Type | Range |
| --- | --- | --- |
| `sleep_timeout` | integer | 1–1440 minutes (positive) |
| `hostname` | string | 1–31 chars, `[A-Za-z0-9-]`, not starting/ending with `-` |
| `volume` | integer | 0–100 percent |
| `welcome` | integer | `0` or `1` (boot welcome animation) |
| `serial_log` | integer | `0` or `1` (USB serial debug logging) |
| `continuous_timeout` | integer | 1–1440 minutes (positive) |
| `loading` | string | `progress` or `sleep_inertia` |
| `access_token` | string | 0–64 printable ASCII; empty string clears the token and disables auth |
| `wifi_ssid` | string | 1–32 chars; setup AP only; must be sent with `wifi_password` |
| `wifi_password` | string | 0–63 chars; setup AP only; empty allowed for open networks; tested before save |
| `servo_mins` | string | Five comma-separated integers `0`–`180` (`head,neck,left,right,body`); setup AP only; must be sent with `servo_maxs` |
| `servo_maxs` | string | Five comma-separated integers `0`–`180`; setup AP only; each value must be greater than the matching min |
| `rgb_order` | string | `RGB`, `RBG`, `GRB`, `GBR`, `BRG`, or `BGR`; setup AP only; default `GRB` |
| `oled_rotate_180` | integer | `0` or `1`; setup AP only; default `0` |

At least one param required. Invalid or missing-all → **400**. WiFi params outside setup AP → **400** `wifi setup only in AP mode`. Servo range params outside setup AP → **400** `servo setup only in AP mode`. RGB order outside setup AP → **400** `rgb setup only in AP mode`. OLED rotation outside setup AP → **400** `oled setup only in AP mode`. WiFi test failure → **400** with error such as `SSID not found`, `Auth failed`, or `Timeout`. `reboot_required` is present and `true` only when the saved hostname differs from the one used at this boot. After successful WiFi save, `wifi_connect_success`, `wifi_ip`, and `wifi_hostname` are included.

### `POST /settings/reset`

Factory reset: clears the NVS `te` namespace and restores settings to compile-time defaults, including WiFi credentials. **Servo min/max, RGB LED mapping (`rgb_order`), and OLED rotation (`oled_rotate_180`) are kept.** After reset, power-cycle (or press reset) so the device reopens setup AP mode and WiFi can be configured again; the servo, screen, and LED steps are pre-filled with the saved values and can be retuned. Until reboot, credentials are cleared in NVS but the radio may still be on the previous STA network. No query params. Requires Bearer when auth is enabled. Same JSON shape as `GET /settings`; includes `reboot_required` when the boot hostname, loading screen, or saved WiFi differed from defaults before reset.

The Config web UI clears the browser-stored Bearer token and shows a one-shot gate: power-cycle the device, then join the robot WiFi shown on the OLED and open the setup wizard. It does not poll for reconnect.

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
  "serial_log": false,
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

Plays `/bell.wav` from LittleFS (`playBell()`). Git source is [`assets/bell.wav`](../assets/bell.wav) (44100 Hz mono PCM); `pio run` copies it into `data/` for the filesystem image. After changing the asset, upload with `pio run -t upload` or `pio run -t uploadfs`. Do not edit `data/` by hand — the next build overwrites it.

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

All five servos: per-joint mid → +0.5 → −0.5 → mid of the saved min/max (`runServoTest()`). Needs a strong 5 V supply — see [power.md](hardware/power.md).

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

### `POST /setup/servo`

Setup AP only. Slowly move one servo, or all five to 90°, using the full **0–180°** electrical range (no saved safe-range clamp). ~25°/s (`SERVO_CALIB_SPEED_DEG_S`). Used by the setup wizard for seating printed parts at 90° and for range finding. Handler blocks until the move finishes. Does **not** require Wi-Fi credentials. Outside provisioning → **400** `setup servo only in AP mode`.

Either `all=90`, or `index` + `angle`.

| Param | Type | Range |
| --- | --- | --- |
| `all` | integer | `90` (every joint to 90°) |
| `index` | integer | `0`–`4` (required with `angle` when `all` is omitted) |
| `angle` | number | `0`–`180` |

```bash
curl -X POST "http://192.168.4.1/setup/servo?all=90"
curl -X POST "http://192.168.4.1/setup/servo?index=0&angle=90"
```

```json
{ "ok": true, "setup": "servo", "index": 0, "angle": 90 }
```

Wrong params return **400** and do not move any servo:

| `error` | When |
| --- | --- |
| `setup servo only in AP mode` | Not in setup AP |
| `invalid all` | `all` present but not `90` |
| `missing index or angle` | Neither `all=90` nor both `index` and `angle` |
| `invalid index` | Non-integer `index` |
| `index out of range` | `index` outside `0`–`4` |
| `invalid angle` | Non-numeric `angle` |
| `angle out of range` | `angle` outside `0`–`180` |

After STA is up, use `POST /test/servo` (clamped to saved min/max).

### `POST /setup/led`

Setup AP only. Holds the LED so provisioning dim-blue does not clobber the preview. Does **not** require Wi-Fi credentials. Outside provisioning → **400** `setup led only in AP mode`.

Logical **color** (`R` / `G` / `B`) lights firmware red/green/blue using the saved `rgb_order`, or `rgb_order` query if sent (wizard remap preview). Wire **byte** lights that WS2812 channel order-independent (`LED_COLOR_ORDER_RGB`, that channel 255). `byte=off` or neither `color` nor `byte` releases the hold and restores dim blue. Do not send `color` and `byte` together.

| Param | Type | Range |
| --- | --- | --- |
| `color` | string | `R`, `G`, or `B` (logical; uses saved or `rgb_order`) |
| `rgb_order` | string | `RGB`, `RBG`, `GRB`, `GBR`, `BRG`, or `BGR` (optional with `color`) |
| `byte` | string | `0`, `1`, `2`, or `off` (omit or `off` releases the hold) |

```bash
curl -X POST "http://192.168.4.1/setup/led?color=R"
curl -X POST "http://192.168.4.1/setup/led?color=R&rgb_order=GRB"
curl -X POST "http://192.168.4.1/setup/led?byte=0"
curl -X POST "http://192.168.4.1/setup/led?byte=off"
```

```json
{ "ok": true, "setup": "led", "color": "R" }
```

Wrong params return **400** and do not change hold state except `off` / omitted light params:

| `error` | When |
| --- | --- |
| `setup led only in AP mode` | Not in setup AP |
| `color and byte conflict` | Both `color` and a numeric `byte` present |
| `invalid color` | `color` not `R`, `G`, or `B` |
| `invalid rgb_order` | `rgb_order` present but not a valid permutation |
| `invalid byte` | `byte` present but not an integer and not `off` |
| `byte out of range` | Integer `byte` outside `0`–`2` |

Save the labeled permutation with `POST /settings?rgb_order=GRB` (setup AP only). The saved order applies immediately to animation colors, boot status, halt blinks, and `POST /test/led`.

### `POST /setup/audio`

Setup AP only. Plays `welcome.wav` from LittleFS (`playWelcome()`, ~2.7 s). Same speaker gain as settings `volume`. No query params. Does **not** require Wi-Fi credentials. Handler blocks until playback finishes. Outside provisioning → **400** `setup audio only in AP mode`. Missing/unreadable WAV → **500** `welcome playback failed`.

```bash
curl -X POST "http://192.168.4.1/setup/audio"
```

```json
{ "ok": true, "setup": "audio" }
```

After STA is up, use `POST /test/audio/bell` (or `/test/audio` for tones).

### `POST /setup/oled`

Setup AP only. Preview OLED 180° rotation without saving. Does **not** require Wi-Fi credentials. Outside provisioning → **400** `setup oled only in AP mode`.

`rotate_180=1` applies `setRotation(2)` and draws `THIS WAY UP`. `rotate_180=0` applies `setRotation(0)` and draws the same test. Omit `rotate_180` to restore saved rotation and the provisioning join-AP text.

| Param | Type | Range |
| --- | --- | --- |
| `rotate_180` | integer | `0` or `1` (omit to restore provisioning text) |

```bash
curl -X POST "http://192.168.4.1/setup/oled?rotate_180=1"
curl -X POST "http://192.168.4.1/setup/oled"
```

```json
{ "ok": true, "setup": "oled", "rotate_180": true }
```

Wrong params return **400**:

| `error` | When |
| --- | --- |
| `setup oled only in AP mode` | Not in setup AP |
| `invalid rotate_180` | `rotate_180` present but not `0` or `1` |

Save with `POST /settings?oled_rotate_180=1` (setup AP only). The saved value applies immediately and on the next boot.

### `POST /test/servo`

Smoothly move one servo to an angle at **~140°/s** (`SERVO_MAX_SPEED_DEG_S`) from its last commanded position. Query params required. Handler blocks until the move finishes.

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

Assembled robot: firmware clamps to the NVS min/max from setup (stock defaults in [robot-movement.md](robot-movement.md)); this route still accepts 0–180 and clamps. For unclamped assembly moves, use setup AP `POST /setup/servo`.

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
| `animation` | `none`, `typing`, `reading`, `thinking`, `ring`, `welcome`, `attention`, `error`, `abort`, `dead`, `wakeup`, or `sleep` |

### `POST /anim`

Request an animation switch. Each animation runs at least **1s**; if the current one is shorter than that, the switch waits until the hold ends. Rapid posts during the hold keep only the **latest** request. Response `animation` is the currently playing name (may still be the old one until the hold ends). Hand moves use speed-limited servo wrappers.

| Param | Type | Values |
| --- | --- | --- |
| `name` | string | `none`, `typing`, `reading`, `thinking`, `ring`, `welcome`, `attention`, `error`, `abort`, `dead`, `wakeup`, `sleep` |

```bash
curl -X POST "http://tiny-engineer.local/anim?name=typing"
curl -X POST "http://tiny-engineer.local/anim?name=reading"
curl -X POST "http://tiny-engineer.local/anim?name=thinking"
curl -X POST "http://tiny-engineer.local/anim?name=ring"
curl -X POST "http://tiny-engineer.local/anim?name=welcome"
curl -X POST "http://tiny-engineer.local/anim?name=wakeup"
curl -X POST "http://tiny-engineer.local/anim?name=sleep"
curl -X POST "http://tiny-engineer.local/anim?name=attention"
curl -X POST "http://tiny-engineer.local/anim?name=error"
curl -X POST "http://tiny-engineer.local/anim?name=abort"
curl -X POST "http://tiny-engineer.local/anim?name=dead"
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
| `dead` | **Hold.** Out-of-power: same obstacle pose + `error.wav` as `error` ("Uh-oh. Human, we have a problem."), but eyes use a failing-display flicker (irregular heights + brief blank pulses, denser near the end) instead of the error nervous scan. When the line ends (or audio fails), eyes squeeze nearly shut (~300 ms) while head/hands collapse, hold shut briefly (~200 ms), then snap to **X X**. Stays in that pose with X eyes and pulsing red until another animation is requested. Does not auto-return to `none`. Requires `error.wav` on LittleFS. |
| `wakeup` | **One-shot** sleep-inertia wake (~5.5 s + settle), same sequence as boot `loading=sleep_inertia`. Eyes open over 2 s (cubic ease) from closed, two blinks at 2.4 s and 3.8 s, head rises from chin-down with a fading neck wave. Always moves head/neck/hands (API path). After completion, `GET /anim` reports `none`. Does not play welcome audio — use `welcome` for that. |
| `sleep` | **One-shot** enter sleep: eye close + head lowers to chin-down (`SLEEP_HEAD_DOWN`, same pose `wakeup` starts from), then blank OLED and `DISPLAYOFF`. Same path as the idle `sleep_timeout`. Holds `sleep` until the head settles; then `GET /anim` reports `none` while the device stays asleep until another non-`sleep`/`none` animation wakes it. |

Wrong params return **400**:

| `error` | When |
| --- | --- |
| `missing name` | Query param `name` absent |
| `unknown animation` | `name` not `none`, `typing`, `reading`, `thinking`, `ring`, `welcome`, `attention`, `error`, `abort`, `dead`, `wakeup`, or `sleep` |

## Errors

| Status | Body | When |
| --- | --- | --- |
| `400` | `{"ok":false,"error":"..."}` | Bad `/test/servo`, `/setup/servo`, `/setup/led`, `/setup/audio`, `/setup/oled`, or `/anim` params (see tables above) |
| `401` | `{"ok":false,"error":"unauthorized"}` | Access token configured and `Authorization: Bearer` missing or wrong |
| `404` | `{"ok":false,"error":"not found"}` | Unknown path |
| `405` | `{"ok":false,"error":"method not allowed"}` | Wrong method on a `/test/*`, `/setup/servo`, `/setup/led`, `/setup/audio`, `/setup/oled`, `/settings`, `/settings/reset`, `/anim`, or `/auth` path |

Test routes are **POST**. GET/prefetch would move hardware. `/anim` allows **GET** (read) and **POST** (set). `/auth` is **GET** only and always public.

## Behaviour

Test handlers **block** until the test finishes. The client waits. After each test, OLED returns to `ROBOT READY` plus IP (or `WIFI FAIL`).

`/anim` responses return immediately; typing motion runs in the main loop via non-blocking servo updates. Animation switches may defer up to 1s so the active animation holds its minimum duration; only the latest pending request is applied. Continuous animations (`typing`, `reading`, `thinking`) that stay active longer than `continuous_timeout` minutes switch to `attention`, which holds ~1 minute then finishes to `none`. Re-`POST`ing the same continuous animation (e.g. `typing` while already `typing`) does not restart motion, but resets the continuous-timeout clock.

The onboard RGB LED follows the active animation (see [RGB LED](#rgb-led)).

One request at a time — the Arduino `WebServer` is single-threaded.

## RGB LED

Onboard WS2812 on **GPIO10** (`RGB_LED_PIN`). Byte order is the saved `rgb_order` setting (default **GRB**). Animation-driven colors are handled in [`src/hardware/rgb.cpp`](../src/hardware/rgb.cpp) and switch when `POST /anim` applies a new state (same 1s minimum hold as servos/eyes).

| Animation | LED color |
| --- | --- |
| `typing`, `reading`, `thinking`, `welcome`, `ring`, `wakeup` | White (full intensity) |
| `attention`, `error`, `dead` | Red pulse: 500 ms 10%→100%, 500 ms hold 100%, 500 ms 100%→10%, repeat |
| `abort` | Red (full intensity, solid) |
| `none`, `sleep` | Off |

Transitions take **1 s** with smooth fade in/out (pulse modes start immediately, no enter fade):

- **Off ↔ color** — single 1 s fade (e.g. idle → typing fades in white; ring → `none` fades out).
- **White ↔ solid red** — fade out to black (500 ms), then fade in to the new color (500 ms).
- **Leaving pulse** — 1 s fade from the current pulse brightness to the new target.

Switching between animations that share the same color (e.g. `typing` → `reading`) does not restart a fade.

Boot uses dim green `(0, 32, 0)` as a status indicator during init. After `ROBOT READY`, the LED fades to white if `welcome` runs (Wi-Fi OK) or off if idle. Fatal PCA9685 / I2S errors hang and blink the LED red (1 = PCA9685 missing, 2 = I2S failed) — not animation-driven. See [boot-failure blink codes](hardware/testing.md#boot-failure-blink-codes).

`POST /test/led` runs a hardware colour cycle and then restores the current animation LED state.
