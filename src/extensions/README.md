# Extensions

Optional hardware add-ons. Keep a clear split from stock robot firmware.

## Layout

| Path | Role |
| --- | --- |
| `src/extensions/<id>/` | Code that exists only for one extension (apply/poll, drivers, that extension’s HTTP handlers) |
| `src/extensions/` root | Shared infra only (`pin_claims*`, HTTP route registry, this README) |
| `src/display/` | Stock OLED / eyes — **not** optional extension displays |

Examples: `push_button/`, `lcd1602/`.

NVS and `GET`/`POST /settings` field plumbing stay in `src/settings/` (project checklist). `main.cpp` only calls each extension’s apply/poll entry points. SPA page: `ui` Extensions view.
