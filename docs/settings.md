# Adding a setting

Persistent settings live in NVS (namespace `te`) under [`src/settings/`](../src/settings/). They are exposed on `GET`/`POST /settings`, the Config web UI, and must stay in sync with API docs.

**HTTP / JSON** (`POST /settings` body and `GET /settings` fields — see [api.md](api.md)): `sleep_timeout`, `hostname`, `volume`, `welcome`, `serial_log`, `push_button`, `push_button_pin`, `push_button_sound_down`, `push_button_sound_up`, `push_button_sound_click`, `push_button_sound_double_click`, `push_button_sound_long_click`, `continuous_timeout`, `loading`, `eyes_style`, `access_token`, `servo_mins` / `servo_maxs`, `rgb_order`, `oled_rotate_180`. WiFi networks have their own endpoint, `PUT /settings/wifi`, and show as `wifi_networks`.

**NVS keys** (≤15 chars; defined in [`nvs.h`](../src/settings/nvs.h)): `sleep_m`, `host`, `vol`, `welcome`, `serial_log`, `ext_btn` (`push_button`), `ext_btn_pin` (`push_button_pin`), `btn_snd_dn` / `btn_snd_up` / `btn_snd_clk` / `btn_snd_dbl` / `btn_snd_lng` (push-button event sounds), `cont_to`, `loading`, `eyes_sty`, `access_tok`, `wifi_nets` (blob of up to 5 networks; replaces the older single-network `wifi_ssid` / `wifi_pass`, which are migrated on load), `sranges`, `rgb_ord`, `oled_rot`. Last commanded servo angles live in the same namespace as key `spose` (internal; not a settings API field). Boot removes a leftover `srv_hold` key from older firmware. Add new short keys there — do not reuse HTTP names as NVS keys unless they already fit the length limit.

Follow the same pattern for a new setting.

## Design choices

| Choice | Convention |
| --- | --- |
| Storage | NVS under namespace `te`; short key name (≤15 chars for Preferences). First boot may create an empty `te` namespace so Preferences does not ERROR on a missing namespace; values stay at defaults until a save |
| RAM | One `SettingsValues` struct (`g_settings`) filled by `initSettings()`; callers use getters |
| Update API | `saveSettings(const SettingsPatch&)`: only the fields the patch sets change, and only changed keys are written to NVS |
| HTTP | JSON object body on `POST /settings`; at least one field required |
| Apply timing | Prefer immediate apply. If boot-only (like hostname), add it to `settingsRebootRequired()` so replies report `reboot_required`, and document it. WiFi networks live in a `WifiNetworkList` saved with `saveWifiNetworks()`, outside `saveSettings(...)`; they are tested before save only in setup AP mode. Servo min/max (`servo_mins` / `servo_maxs`), RGB LED mapping (`rgb_order`), and OLED rotation (`oled_rotate_180`) are setup-AP-only and apply immediately |
| Types | Prefer small integers / short strings; validate before any NVS write |

## Checklist

Work through these layers in order. Mirror an existing setting (`volume` is the simplest integer example; `hostname` shows reboot + string validation).

### 1. Core — [`src/settings/`](../src/settings/)

Public API: [`settings.h`](../src/settings/settings.h). Values struct: [`values.h`](../src/settings/values.h) / [`values.cpp`](../src/settings/values.cpp). NVS keys, load, save and reset: [`nvs.h`](../src/settings/nvs.h) / [`nvs.cpp`](../src/settings/nvs.cpp). Boot: [`load.cpp`](../src/settings/load.cpp). Getters: [`getters.cpp`](../src/settings/getters.cpp). Validation: [`validate.cpp`](../src/settings/validate.cpp).

1. Add `SETTINGS_DEFAULT_*`, min/max (or length) constants in [`settings.h`](../src/settings/settings.h).
2. Add the field to `SettingsValues` in [`values.h`](../src/settings/values.h), set its default in `SettingsValues::defaults()` and log it in `SettingsValues::log()` ([`values.cpp`](../src/settings/values.cpp)). For secrets like `access_token`, log set/unset only, never the raw value.
3. Add the NVS key in [`nvs.h`](../src/settings/nvs.h) (e.g. `kKeyFoo = "foo"`). Read it in `SettingsValues::load()` (keep the default on bad or missing data) and write it in `writeValues()` ([`nvs.cpp`](../src/settings/nvs.cpp)).
4. Add `settingsValidateFoo(...)` in [`validate.cpp`](../src/settings/validate.cpp) and the getter `settingsFoo()` in [`getters.cpp`](../src/settings/getters.cpp).
5. Add the field to `SettingsPatch` in [`settings.h`](../src/settings/settings.h) (`std::optional<T>`, or `const char*` for text), check it in `SettingsPatch::invalidField()` and copy it in `SettingsValues::apply()`. `saveSettings()` rejects the whole patch when a field is invalid and writes only the keys whose value changed.

### 2. Consumers

Call the getter wherever the value affects behavior (e.g. `settingsVolume()` in audio). Prefer reading the getter at use time so `POST /settings` applies without reboot.

### 3. HTTP — [`src/http/settings_handlers.cpp`](../src/http/settings_handlers.cpp)

1. Add the field to `settingsJson()`.
2. Read it into the patch with one `read("foo", patch.foo)` line in `readPatch()`. A wrong JSON type answers **400** `invalid <field>`, and so does a value `SettingsPatch::invalidField()` rejects. Fields `readPatch()` does not read answer **400** `unknown field <field>`.

### 4. Web UI — [`ui/`](../ui/)

1. Add the field to `Settings` in [`ui/src/api/types.ts`](../ui/src/api/types.ts).
2. Add a control to `ConfigForm` in [`ui/src/config/ConfigForm.tsx`](../ui/src/config/ConfigForm.tsx): read it in `draftFrom()` and send it from `save()`. Use `.range-row` + range/number for 0–N scales (see volume). Exception: extension toggles, pin, and sound fields such as `push_button` / `push_button_pin` / `push_button_sound_*` live on the Extensions page instead of Config.
3. API reference table under POST `/settings` in [`ui/src/views/ApiReference.tsx`](../ui/src/views/ApiReference.tsx): param, type, range.
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
| API | `"volume":70` on GET; `POST /settings` `{"volume":40}` |
| UI | Slider + number on Config; live `N%` label |
| Docs | `api.md` + README |

## Pitfalls

- **Partial writes:** validate first; never write NVS then fail validation mid-way.
- **Factory reset:** `factoryResetSettings()` in [`nvs.cpp`](../src/settings/nvs.cpp) clears NVS namespace `te` and writes defaults, which clears every saved WiFi network. Servo min/max (`sranges`), RGB LED mapping (`rgb_ord`), and OLED rotation (`oled_rot`) are written back unchanged. Internal pose blob (`spose`) is not rewritten (next boot uses mid). Exposed as `POST /settings/reset`. After reset, power-cycle into setup AP mode to configure WiFi again. Servo ranges, LED mapping, and screen rotation can be retuned in that wizard.
- **Hostname-style settings:** freeze the boot value separately if live change cannot apply (see `g_bootSettings` and `settingsRebootRequired()`).
- **UI size:** the panel is embedded gzipped in the firmware; keep controls compact.
- **Doc drift:** HTML param tables must match `api.md` exactly.

## Related

- Runtime API: [`api.md`](api.md) (`GET`/`POST /settings`, `POST /settings/reset`)
- Endpoint sync rule: [`.cursor/rules/sync-api-endpoints.mdc`](../.cursor/rules/sync-api-endpoints.mdc)
