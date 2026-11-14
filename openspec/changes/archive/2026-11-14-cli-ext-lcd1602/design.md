# Design

## Context

HTTP already has `http_registry` (`ExtHttpRoute` table) for `/extensions/<id>/…`. LCD handlers live under `src/extensions/lcd1602/http.*`. Serial `link.cpp` dispatches JSON `op`s; `settings.get`/`settings.set` omit `lcd1602*`. CLI `ops.js` + `viaTransport` already unify HTTP/serial for settings/test/servo/park/reset; `anim`/`play` use `transport.js`. No `ext` command yet.

## Goals / Non-Goals

**Goals:**
- One serial `ext` dispatcher sharing the HTTP extension mount table
- LCD 1602 as first (only) registered serial/HTTP consumer in this change
- Serial settings slice for `lcd1602*` so USB-only can enable the extension
- CLI `tiny-engineer ext <id> …` with transport parity for LCD text/brightness

**Non-Goals:**
- Full serial `settings.get` parity with every `GET /settings` field (wifi, actions, …)
- Serial/CLI surface for push-button or other extensions beyond registering future rows
- Short `lcd` CLI alias
- Bearer auth on serial (unchanged: serial skips token)

## Decisions

### 1. Serial op shape: registry mirror (B)
Request fields: `op: "ext"`, `id` (e.g. `lcd1602`), `suffix` (e.g. `text` / `brightness`), `method` (`GET`|`POST`|`DELETE`), plus body fields for POST (same JSON keys as HTTP). Dispatch: look up `(path=/extensions/<id>/<suffix>, method)` in the shared table; run `Refusal::check(Configured)` then `enabled()` then handler. Prefer extracting a small shared invoke helper used by HTTP `server.on` lambdas and serial, rather than duplicating LCD logic.

**Alternatives rejected:** dedicated `lcd1602.text` ops (does not match CLI `ext`); raw HTTP path tunnel (heavier framing).

### 2. CLI is generic `ext`; unknown id fails locally
CLI knows a small map of ids → resources (v1: only `lcd1602`). Unknown id → exit 1 before open/HTTP. Known id + resource maps to HTTP path or serial `ext` fields via `viaTransport`-style helper.

Example UX:
```
tiny-engineer ext lcd1602 text get
tiny-engineer ext lcd1602 text set --line1 HELLO --align center
tiny-engineer ext lcd1602 text clear
tiny-engineer ext lcd1602 brightness get|set <n>
```
Enable/boot: existing `settings set --lcd1602=true` (after serial settings slice).

### 3. Settings slice only
`settings.get` adds the five `lcd1602*` fields. `settings.set` accepts them with existing validators and calls `lcd1602ApplyFromSettings` when relevant (same as HTTP handler). Document in `docs/serial.md` that other settings gaps remain.

### 4. Error parity
Reuse HTTP error strings where handlers already emit them (`extension disabled`, `lcd not found`, `backlight not found`, validation messages). Serial status codes mirror HTTP (`code` in JSON res).

### 5. Docs / tests
Update `docs/serial.md` ops list; CLI help/README. CLI unit tests for arg parsing / unknown id; firmware: extend serial settings coverage if native-testable, else compile + focused serial/HTTP smoke notes in tasks.

## Risks / Trade-offs

- [Shared handler coupling] → Keep invoke thin; handlers stay WebServer-oriented — may need a small adapter (fake WebServer or body/response callbacks). Prefer response JSON builders already used by LCD HTTP if refactor stays small.
- [Partial settings parity vs main serial-protocol wording] → This change MODIFIES the parity requirement to admit the lcd slice + `ext` while naming the remaining gap explicitly.
- [CLI id allowlist drift] → Document that new extensions add CLI map entries + registry rows together.

## Migration Plan

1. Firmware: settings lcd fields on serial; then `ext` dispatcher wired to existing LCD routes.
2. CLI: `ext` command + ops; tests; docs.
3. No NVS migration. Rollback = prior firmware/CLI; unknown `ext` op → 404 as today.

## Open Questions

None blocking. Exact CLI flag names for line2/align can follow existing `settings set --key=value` style where practical.
