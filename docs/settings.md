# Adding a setting

Persistent settings live in NVS (namespace `te`) under [`src/settings/`](../src/settings/). They are exposed on `GET`/`POST /settings`, the Config web UI, and must stay in sync with API docs.

**HTTP / JSON** (`POST /settings` body and `GET /settings` fields — see [api.md](api.md)): `sleep_timeout`, `hostname`, `volume`, `welcome`, `serial_log`, `continuous_timeout`, `loading`, `eyes_style`, `access_token`, `servo_mins` / `servo_maxs` / `servo_centers`, `swap_hands`, `rgb_order`, `oled_rotate_180`. WiFi networks have their own endpoint, `PUT /settings/wifi`, and show as `wifi_networks`.

**NVS keys** (≤15 chars; defined in [`nvs.h`](../src/settings/nvs.h)): `sleep_m`, `host`, `vol`, `welcome`, `serial_log`, `cont_to`, `loading`, `eyes_sty`, `access_tok`, `wifi_nets` (blob of up to 5 networks; replaces the older single-network `wifi_ssid` / `wifi_pass`, which are migrated on load), `sranges`, `scenters` (five float center angles), `swap_hands`, `rgb_ord`, `oled_rot`. Add new short keys there — do not reuse HTTP names as NVS keys unless they already fit the length limit.

Follow the same pattern for a new setting.

## Design choices

| Choice | Convention |
| --- | --- |
| Storage | NVS under namespace `te`; short key name (≤15 chars for Preferences) |
| RAM | One `SettingsValues` struct (`g_settings`) filled by `initSettings()`; callers use getters |
| Update API | `saveSettings(const SettingsPatch&)`: only the fields the patch sets change, and only changed keys are written to NVS |
| HTTP | JSON object body on `POST /settings`; at least one field required |
| Apply timing | Prefer immediate apply. If boot-only (like hostname or hand channel assignment), add it to `settingsRebootRequired()` so replies report `reboot_required`, and document it. Servo centers (`servo_centers`) apply to subsequent normalized moves without reboot and can be set outside setup AP. WiFi networks live in a `WifiNetworkList` saved with `saveWifiNetworks()`, outside `saveSettings(...)`; they are tested before save only in setup AP mode. Servo min/max (`servo_mins` / `servo_maxs`) can also be saved outside setup AP and apply immediately. RGB LED mapping (`rgb_order`) and OLED rotation (`oled_rotate_180`) are setup-AP-only and apply immediately |
| Types | Prefer small integers / short strings; validate before any NVS write |

## Checklist

Work through these layers in order. Mirror an existing setting (`volume` is the simplest integer example; `hostname` shows reboot + string validation).

### 1. Core — [`src/settings/`](../src/settings/)

Public API: [`settings.h`](../src/settings/settings.h). Values struct: [`values.h`](../src/settings/values.h) / [`values.cpp`](../src/settings/values.cpp). NVS keys, load, save and reset: [`nvs.h`](../src/settings/nvs.h) / [`nvs.cpp`](../src/settings/nvs.cpp). Boot: [`load.cpp`](../src/settings/load.cpp). Getters: [`getters.cpp`](../src/settings/getters.cpp). Validation: [`validate.cpp`](../src/settings/validate.cpp).

1. Add `SETTINGS_DEFAULT_*`, min/max (or length) constants in [`settings.h`](../src/settings/settings.h).
2. Add the field to `SettingsValues` in [`values.h`](../src/settings/values.h), set its default in `SettingsValues::defaults()` and log it in `SettingsValues::log()` ([`values.cpp`](../src/settings/values.cpp)). For secrets like `access_token`, log set/unset only, never the raw value.
3. Add the NVS key in [`nvs.h`](../src/settings/nvs.h) (e.g. `kKeyFoo = "foo"`). Read it in `SettingsValues::load()` (keep the default on bad or missing data) and write it in `writeValues()` ([`nvs.cpp`](../src/settings/nvs.cpp)).
4. Add `settingsValidateFoo(...)` in [`validate.cpp`](../src/settings/validate.cpp) and the getter `settingsFoo()` in [`getters.cpp`](../src/settings/getters.cpp).
5. Add the field to `SettingsPatch` in [`settings.h`](../src/settings/settings.h) (`std::optional<T>`, or `const char*` for text), check it in `SettingsPatch::invalidField()` and copy it in `SettingsValues::apply()`. `saveSettings()` rejects the whole patch when a field is invalid (centers are checked against the resulting safe ranges) and writes only the keys whose value changed.

### 2. Consumers

Call the getter wherever the value affects behavior (e.g. `settingsVolume()` in audio). Prefer reading the getter at use time so `POST /settings` applies without reboot.

### 3. HTTP — [`src/http/settings_handlers.cpp`](../src/http/settings_handlers.cpp)

1. Add the field to `settingsJson()`.
2. Read it into the patch with one `read("foo", patch.foo)` line in `readPatch()`. A wrong JSON type answers **400** `invalid <field>`, and so does a value `SettingsPatch::invalidField()` rejects. Fields `readPatch()` does not read answer **400** `unknown field <field>`.

### 4. Web UI — [`ui/`](../ui/)

1. Add the field to `Settings` in [`ui/src/api/types.ts`](../ui/src/api/types.ts).
2. Add a control to `ConfigForm` in [`ui/src/config/ConfigForm.tsx`](../ui/src/config/ConfigForm.tsx): read it in `draftFrom()` and send it from `save()`. Use `.range-row` + range/number for 0–N scales (see volume).
3. API reference table under POST `/settings` in [`ui/src/views/ApiReference.tsx`](../ui/src/views/ApiReference.tsx): param, type, range.
4. Home card blurb if the setting is user-facing. Servo calibration lives on Servo control and in the setup wizard instead of ConfigForm. Hand channel assignment lives on Config and in the setup wizard.

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
| API | `"volume":70` on GET; `POST /settings` `{"volume":40}` |
| UI | Slider + number on Config; live `N%` label |
| Docs | `api.md` + README |

## Pitfalls

- **Partial writes:** validate first; never write NVS then fail validation mid-way.
- **Factory reset:** `factoryResetSettings()` in [`nvs.cpp`](../src/settings/nvs.cpp) clears NVS namespace `te` and writes defaults, which clears every saved WiFi network. Servo min/max (`sranges`), centers (`scenters`), hand channel assignment (`swap_hands`), RGB LED mapping (`rgb_ord`), and OLED rotation (`oled_rot`) are written back unchanged. Exposed as `POST /settings/reset`. After reset, power-cycle into setup AP mode to configure WiFi again. Servo ranges and centers, LED mapping, and screen rotation can be retuned in that wizard. Limits and centers can also be retuned on Servo control outside setup AP.
- **Hand assignment:** `swap_hands` selects left=3/right=2 instead of the default left=2/right=3. PWM uses the mapping from `g_bootSettings` until the next restart; changing the saved value sets `reboot_required`. Keep this mapping in factory reset.
- **Servo center migration:** missing or invalid `scenters` defaults to the midpoint of the loaded safe ranges. If a range-only patch excludes an existing center, that center falls back to the new midpoint. Explicit centers must be finite and strictly inside the resulting ranges; reject before writing NVS.
- **Hostname-style settings:** freeze the boot value separately if live change cannot apply (see `g_bootSettings` and `settingsRebootRequired()`).
- **UI size:** the panel is embedded gzipped in the firmware; keep controls compact.
- **Doc drift:** HTML param tables must match `api.md` exactly.

## Related

- Runtime API: [`api.md`](api.md) (`GET`/`POST /settings`, `POST /settings/reset`)
- Endpoint sync rule: [`.cursor/rules/sync-api-endpoints.mdc`](../.cursor/rules/sync-api-endpoints.mdc)
