# Design

## Context

See proposal.md — Why. Today:

- `eventsWebSocketUrl` / `redactEventsUrl` live in `commands/watch.js`; `daemon/reactions.js` imports them (daemon → command).
- `robot/ops.js` has private `withTransport` (resolve → `serialRequest` or `httpFn`).
- `robot/transport.js` repeats the same fork in `robotSetAnim`, `robotPlay`, `robotHealth`, `robotOp`; `robotPlay` uses `serialPlay` instead of `serialRequest`.

`skip_specs: true` — behavior stays as today; `cli-transport` already covers serial-via-daemon.

## Goals / Non-Goals

**Goals:**

- Break daemon → command dependency for events URL helpers.
- One shared HTTP/serial router for ops + transport entry points.
- Keep play’s serial path (`serialPlay` + wavPath) without special-casing the helper for every op.

**Non-Goals:**

- `port --probe` via daemon (5b).
- Changing daemon IPC, CLI flags, or HTTP/serial wire formats.
- Merging `ops.js` and `transport.js` into one file.
- Re-homing unrelated watch parsing / `runWatch`.

## Decisions

### 1. Events URL module: `robot/events-url.js`

**Choice:** New `packages/tiny-engineer-cli/src/robot/events-url.js` holding both helpers (move, not copy). `commands/watch.js` and `daemon/reactions.js` import from there. Tests import the new module (or re-export only if needed for public API — prefer direct imports).

**Why:** URL building is robot/HTTP concern, not a CLI command. `robot/` already owns client/transport/serial.

**Alternatives:** `util/events-url.js` (weaker ownership); re-export from `watch.js` for compat (keeps the wrong home in the public surface — skip).

### 2. Shared `withTransport` home: `robot/with-transport.js`

**Choice:** Extract the resolve + serial/http fork from `ops.js` into `robot/with-transport.js`. Both `ops.js` and `transport.js` import it.

**Signature (conceptual):**

```js
withTransport(opts, { serial, http })
```

- `serial`: async () => result — default in callers: wrap `serialRequest(op, fields, { port })` with existing ok/error mapping.
- `http`: async () => result — existing HTTP closures.
- On resolve error: `{ ok: false, error }` (same as today).
- No `target` injection inside the helper — callers that need `target: "serial" | baseUrl` add it after (preserves health’s `detail` shape and play’s body).

**Why separate file:** Avoids `ops` ↔ `transport` import cycle and keeps the router free of settings/anim specifics.

**Alternatives:** Export from `ops.js` and import in `transport.js` (couples “settings ops” to anim/play); put helper only in `transport.js` and have ops import (same coupling, wrong name). Rejected.

### 3. Play uses custom `serial` callback

**Choice:** `robotPlay` passes a `serial` function that checks `wavPath`, calls `serialPlay`, maps errors — not forced through `serialRequest`.

**Why:** Play is the only op with a different daemon IPC (`serialPlay`). Extending the helper with optional `serial` keeps anim/health/robotOp/ops on the default path without a play-specific branch inside the helper.

### 4. Health stays detail-shaped

**Choice:** `robotHealth` keeps returning `{ ok, detail, target? }`. Helper only chooses serial vs http; health maps serial failures into `detail` as today.

**Why:** Avoid a breaking shape change for status/ping callers for a refactor.

## Risks / Trade-offs

- [Subtle behavior drift while collapsing forks] → Diff each call site against current mapping; run full `npm test --prefix packages/tiny-engineer-cli`.
- [Import cycles if events-url pulls daemon/watch] → Keep `events-url.js` pure (URL only, no daemon/command imports).
- [Future ops forget the helper] → Document in a one-line comment on `with-transport.js`; lint/review, not a new eslint rule in this change.

## Migration Plan

In-tree refactor only: move helpers, wire imports, collapse forks, update tests. No release notes / NVS / firmware. Rollback = revert the commit.

## Open Questions

None — 5b deferred by proposal; module paths fixed above.
