# Tasks

## 1. Events URL extract (5a)

- [x] 1.1 Add `packages/tiny-engineer-cli/src/robot/events-url.js` with `eventsWebSocketUrl` and `redactEventsUrl` moved from `commands/watch.js` (same behavior); verify module exports both and has no imports from `commands/` or `daemon/`
- [x] 1.2 Update `commands/watch.js` and `daemon/reactions.js` to import from `robot/events-url.js`; remove the helpers from `watch.js`; verify `rg "eventsWebSocketUrl|redactEventsUrl" packages/tiny-engineer-cli` shows no definitions under `commands/` and no `reactions` → `watch` import
- [x] 1.3 Point `test/watch.test.js` (or a dedicated `events-url` test) at the new module; verify `npm test --prefix packages/tiny-engineer-cli -- test/watch.test.js` (or the new test file) passes

## 2. Shared withTransport (5c)

- [x] 2.1 Add `packages/tiny-engineer-cli/src/robot/with-transport.js` exporting `withTransport(opts, { serial, http })` as in design.md; verify resolve errors return `{ ok: false, error }` and serial/http callbacks are invoked for the matching transport
- [x] 2.2 Refactor `robot/ops.js` to use the shared helper for all existing ops; verify existing ops/settings-related tests still pass (`npm test --prefix packages/tiny-engineer-cli` filtered to ops/settings if present, else note reliance on full suite in 3.1)
- [x] 2.3 Refactor `robot/transport.js` (`robotSetAnim`, `robotPlay`, `robotHealth`, `robotOp`) through the shared helper; `robotPlay` supplies a custom `serial` callback using `serialPlay`; preserve `target` / `detail` shapes; verify no remaining local `resolveTransport` + serial forks in those four functions (`rg` / read file)

## 3. Integration check

- [x] 3.1 Run `npm test --prefix packages/tiny-engineer-cli` and fix any regressions from the moves/refactors

## Workflow follow-up

- After apply + verify: `/opsx-archive` (with sync if needed)
- Separate change later for **5b** (`port --probe` via daemon) — not this change
