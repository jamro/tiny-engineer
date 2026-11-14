# Proposal

## Why

USB-only and serial-first hosts can drive anim/play/settings today, but LCD 1602 lives only on HTTP `/extensions/lcd1602/…`. The CLI has no extension commands, and serial `settings.get`/`settings.set` omit the `lcd1602*` fields needed to enable the module. Users need the same LCD control over `--transport serial` as over HTTP, with a CLI shape that scales to later extensions.

## What Changes

- Add a **serial `ext` dispatcher** that mounts the same extension route table as HTTP (`id` + `suffix` + method + JSON body), with **LCD 1602 as the only registered consumer** in this change
- Extend serial **`settings.get` / `settings.set`** to include/accept the **`lcd1602*` settings slice** (enable + boot/backlight/lines) so the extension can be turned on without Wi‑Fi
- Add CLI command **`tiny-engineer ext <id> …`** (first id: `lcd1602`) for text get/set/clear and brightness get/set over HTTP or serial via existing transport helpers
- Document serial `ext` and CLI usage; update help/`docs/serial.md`
- **Not in this change:** full serial settings parity (wifi/actions/all HTTP fields), push-button (or other) extension APIs over serial, a short `lcd` CLI alias

## Capabilities

### New Capabilities
- `extension-serial`: Serial JSON `op: "ext"` that reuses the shared extension mount table (same id/suffix/method semantics and refusals as HTTP `/extensions/<id>/…`)

### Modified Capabilities
- `serial-protocol`: Add `ext` to the control surface; narrow settings get/set to also carry the `lcd1602*` field slice (full GET `/settings` parity remains deferred)
- `cli-transport`: Add day-to-day `ext <id>` commands with transport parity for LCD 1602 resources
- `lcd1602`: Clarify that text/brightness APIs are reachable via the serial `ext` path with the same behavior as HTTP (disabled/503, validation, last-state)

## Impact

- Firmware: `src/serial/link.cpp` (and shared helpers to dispatch into `http_registry` / LCD handlers); serial settings JSON include/accept `lcd1602*`
- CLI: `packages/tiny-engineer-cli` — new `ext` command, `ops.js` (or sibling) for HTTP+serial, help/README tests
- Docs: `docs/serial.md`, CLI help; API docs cross-link if needed
- Specs: new `extension-serial`; deltas on `serial-protocol`, `cli-transport`, `lcd1602`
- Scope `integrations` for package work; firmware/serial for device side
