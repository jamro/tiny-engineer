# Adding a setting

Persistent settings live in NVS (namespace `te`) under [`src/settings/`](../src/settings/). They are exposed on `GET`/`POST /settings`, the Config web UI, and must stay in sync with API docs.

Existing keys: `sleep_timeout`, `hostname`, `volume`, `welcome`, `serial_log`, `continuous_timeout`, `loading`, `access_token`, `wifi_ssid`, `wifi_password`, `sranges` (packed servo min/max blob), `rgb_ord` (WS2812 byte-order string), `oled_rot` (OLED 180° rotation bool). Follow the same pattern for a new one.

## Design choices

| Choice | Convention |
| --- | --- |
| Storage | NVS under namespace `te`; short key name (≤15 chars for Preferences) |
| RAM | Cached in a module-static after `initSettings()`; callers use getters |
| Update API | Nullable pointer args to `saveSettings(...)` — only non-null fields change |
| HTTP | Query params on `POST /settings`; at least one param required |
| Apply timing | Prefer immediate apply. If boot-only (like hostname), set `reboot_required` and document it. WiFi credentials are setup-AP-only and tested before save. Servo min/max (`servo_mins` / `servo_maxs`), RGB LED mapping (`rgb_order`), and OLED rotation (`oled_rotate_180`) are setup-AP-only and apply immediately |
| Types | Prefer small integers / short strings; validate before any NVS write |

## Checklist

Work through these layers in order. Mirror an existing setting (`volume` is the simplest integer example; `hostname` shows reboot + string validation).

### 1. Core — [`src/settings/`](../src/settings/)

Public API: [`settings.h`](../src/settings/settings.h). RAM cache: [`internal.h`](../src/settings/internal.h) / [`cache.cpp`](../src/settings/cache.cpp). NVS keys: [`nvs.h`](../src/settings/nvs.h) / [`nvs.cpp`](../src/settings/nvs.cpp). Load: [`load.cpp`](../src/settings/load.cpp). Save: [`save.cpp`](../src/settings/save.cpp). Reset: [`reset.cpp`](../src/settings/reset.cpp). Getters: [`getters.cpp`](../src/settings/getters.cpp). Validation: [`validate.cpp`](../src/settings/validate.cpp).

1. Add `SETTINGS_DEFAULT_*`, min/max (or length) constants in [`settings.h`](../src/settings/settings.h).
2. Add NVS key string in [`nvs.h`](../src/settings/nvs.h) (e.g. `kKeyFoo = "foo"`) and persist it in `writeAllToNvs` / `settingsNvsPutAll` in [`nvs.cpp`](../src/settings/nvs.cpp).
3. Add RAM cache variable in [`internal.h`](../src/settings/internal.h) / [`cache.cpp`](../src/settings/cache.cpp); load + validate in `initSettings()` ([`load.cpp`](../src/settings/load.cpp)); fall back to default on bad/missing data.
4. Add getter `settingsFoo()` in [`getters.cpp`](../src/settings/getters.cpp) and `settingsValidateFoo(...)` in [`validate.cpp`](../src/settings/validate.cpp).
5. Extend `saveSettings(...)` in [`save.cpp`](../src/settings/save.cpp) with a nullable `const T* foo`:
   - Reject the whole save if validation fails.
   - Require at least one non-null arg among all settings.
   - Write all persisted fields together (current pattern rewrites sleep/host/volume/welcome/continuous_timeout/loading/access_token each save).
6. Log the new value on load and save via `logSettingsSnapshot` in [`cache.cpp`](../src/settings/cache.cpp) (for secrets like `access_token`, log set/unset only — never the raw value).

### 2. Consumers

Call the getter wherever the value affects behavior (e.g. `settingsVolume()` in audio). Prefer reading the getter at use time so `POST /settings` applies without reboot.

### 3. HTTP — [`src/http/settings_handlers.cpp`](../src/http/settings_handlers.cpp)

1. Include the field in both `snprintf` branches of `sendSettingsJson` (with and without `reboot_required`).
2. Grow the JSON buffer if the payload no longer fits.
3. In `handleSettingsPost`:
   - `server.hasArg("foo")`
   - Parse (same style as `sleep_timeout` / `volume`: `strtoul` + end-pointer check for integers)
   - Validate → **400** with a clear `error` string
   - Pass pointer into `saveSettings`
4. Update the “missing …” **400** message to list the new param.

### 4. Web UI — [`src/http/index_page.cpp`](../src/http/index_page.cpp)

1. Config form control (`#config-foo`). Use `.range-row` + range/number for 0–N scales (see volume).
2. `loadSettings()` reads `j.foo`; save POST includes `&foo=…`.
3. API reference table under POST `/settings`: param, type, range.
4. Home Config card blurb if the setting is user-facing.

### 5. Docs (required with the same change)

Per [`.cursor/rules/sync-api-endpoints.mdc`](../.cursor/rules/sync-api-endpoints.mdc):

1. [`docs/api.md`](api.md) — GET example JSON, field table, POST curl + param table, apply timing.
2. [`README.md`](../README.md) — short mention if the Config/settings summary lists settings.
3. [`docs/hardware/testing.md`](hardware/testing.md) — settings rows in the route table if present.

### 6. Build

```bash
pio run
```

Flash only when you want to try it on hardware (`pio run -t upload`).

## Example: integer percent (`volume`)

| Layer | What landed |
| --- | --- |
| Settings | `SETTINGS_DEFAULT_VOLUME = 70`, key `vol`, `settingsVolume()`, range 0–100 |
| Audio | Tone amplitude and WAV PCM scaled by `settingsVolume() / 100` |
| API | `"volume":70` on GET; `POST /settings?volume=40` |
| UI | Slider + number on Config; live `N%` label |
| Docs | `api.md` + README |

## Pitfalls

- **Partial writes:** validate first; never write NVS then fail validation mid-way.
- **JSON buffer:** `sendSettingsJson` uses a fixed `char` buffer — bump size when adding fields.
- **Factory reset:** `factoryResetSettings()` in [`reset.cpp`](../src/settings/reset.cpp) clears NVS namespace `te` and writes defaults, including WiFi credentials. Servo min/max (`sranges`), RGB LED mapping (`rgb_ord`), and OLED rotation (`oled_rot`) are written back unchanged. Exposed as `POST /settings/reset`. After reset, power-cycle into setup AP mode to configure WiFi again (WiFi is not editable on the normal Config page). Servo ranges, LED mapping, and screen rotation can be retuned in that wizard.
- **Hostname-style settings:** freeze the boot value separately if live change cannot apply (see `settingsBootHostname()` / `reboot_required`).
- **HTML string size:** the panel is a big string literal in `index_page.cpp`; keep controls compact.
- **Doc drift:** HTML param tables must match `api.md` exactly.

## Related

- Runtime API: [`api.md`](api.md) (`GET`/`POST /settings`, `POST /settings/reset`)
- Endpoint sync rule: [`.cursor/rules/sync-api-endpoints.mdc`](../.cursor/rules/sync-api-endpoints.mdc)
