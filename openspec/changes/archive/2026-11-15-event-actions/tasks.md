# Tasks

## 1. Actions storage and match

- [x] 1.1 Add Action struct list (max 16), NVS key, load/save/validate helpers mirroring wifi networks; verify factory reset leaves Actions empty and `pio test -e native` covers validate/load edge cases that fit native
- [x] 1.2 Implement `*` pattern matcher (type string only) with unit tests for exact, prefix/suffix/mid, multi-star, and non-match; verify `pio test -e native` passes
- [x] 1.3 Add legacy `push_button_sound_*` → Actions migration on load (empty list only) and clear legacy NVS keys; verify migration cases with native or focused tests

## 2. Dispatch and effects

- [x] 2.1 Add `actionsDispatch(type)` that fires all matching rows in order; hook it from `publishEvent`; remove push-button inline SFX call; verify button publish still emits WS/serial and no longer depends on `push_button_sound_*`
- [x] 2.2 Wire sound Actions to existing LittleFS bell/click start path with the same busy/preempt rules; verify idle starts clip and busy skips
- [x] 2.3 Wire anim Actions through `apiSetAnimation` (same as `POST /anim`); verify known name starts anim and unknown is skipped without blocking later rows
- [x] 2.4 ~~Webhook~~ dropped — Actions are sound/anim only

## 3. HTTP API

- [x] 3.1 Add `GET /settings/actions` and `PUT /settings/actions` (Token) replace-all with validation errors; register in `kRoutes`; verify empty/list/replace/over-max/invalid-row behaviors
- [x] 3.2 Add `POST /actions/test` (Configured) simulating `type` without outbound publish; verify matching Actions run and WS/serial do not get the simulated envelope
- [x] 3.3 Remove `push_button_sound_*` from settings GET/POST/validate/docs path; verify unknown/legacy field posts no longer accepted and GET omits them
- [x] 3.4 Update `docs/api.md`, ApiReference, and any README/hardware testing route summaries for Actions endpoints and sound-field removal; verify tables match handlers

## 4. Web UI

- [x] 4.1 Add Actions page (`/actions`) with clear multi-row editor (pattern, kind, params, add/remove), Save via `PUT /settings/actions`, and simulate-event control via `POST /actions/test`; register nav + route; verify `npm run lint --prefix ui` and `npm test --prefix ui`
- [x] 4.2 Strip Extensions sound selects and `push_button_sound_*` posts; keep event catalog + pointer to Actions; update types/tests; verify Extensions save only enable/pin fields
- [x] 4.3 Extract shared anim name list for Actions + Animations pickers; verify both UIs show the same names

## 5. Integration check

- [x] 5.1 Run `pio run` and fix compile failures from this change
- [x] 5.2 Smoke: save Actions (sound + anim pattern), simulate event from UI/API, confirm effects; optional real button path when hardware available
