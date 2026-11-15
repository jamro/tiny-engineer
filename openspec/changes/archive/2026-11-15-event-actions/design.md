# Design

## Context

See proposal.md — Why. Today `publishEvent` in `src/events/hub.cpp` fans out to WebSocket + serial only. Push-button is the sole producer and calls `tryStartButtonEventSound` beside publish. Settings lists use typed NVS blobs (wifi pattern). Host `rules.json` stays untouched.

## Goals / Non-Goals

**Goals:**

- Central Actions list + wildcard match + sound/anim effects, hooked from `publishEvent`
- Replace-all HTTP API + Actions UI + simulate-event test
- Migrate legacy `push_button_sound_*` once, then delete those settings paths
- Keep anim behavior identical to `POST /anim` via `apiSetAnimation`

**Non-Goals:**

- Host/CLI/`rules.json` changes or parity with host wildcards
- Webhook / outbound HTTP client Actions
- Publishing synthetic outbound envelopes from simulate
- New event producers beyond existing push-button

## Decisions

### 1. Hook dispatch inside `publishEvent`

Call `actionsDispatch(type)` at the end of `publishEvent` after WS/serial emit. Push-button drops its inline SFX call. Simulate calls `actionsDispatch` only (no envelope).

**Why:** One path for every future producer; avoids forgetting Actions beside a second `publishEvent` call site.

**Alt:** Keep SFX in push-button and only run anim from hub — rejected; duplicates sound policy and fights “Actions replace sounds”.

### 2. Storage mirrors wifi networks

Fixed `Action` struct array, max **16**, NVS key `actions` (≤15), `putBytes` blob, `saveActions` / `loadActions`, separate from `SettingsPatch`. RAM: `g_actions` list.

Rough layout per row (packed carefully for flash):

- `on[48]` pattern
- `do` enum (`sound` / `anim`)
- params: `sound` enum or `anim[16]`

Validate whole list before write. Factory reset clears Actions with other `te` keys (except preserved servo/oled blobs per existing reset rules — Actions are user config, clear them).

**Why:** Matches project settings guidance; avoids unbounded JSON-in-NVS.

**Alt:** JSON string in Preferences — rejected (size/fragmentation, harder validate).

### 3. HTTP surface

| Method | Path | Access | Body |
| --- | --- | --- | --- |
| GET | `/settings/actions` | Token | — → `{ "actions": [...] }` |
| PUT | `/settings/actions` | Token | `{ "actions": [ ... ] }` replace-all |
| POST | `/actions/test` | Configured | `{ "type": "…" }` |
| GET | `/actions` | Public | SPA shell (`handleIndex`), same as `/extensions` |

JSON API cannot share GET `/actions` with the SPA page route. List CRUD mirrors `PUT /settings/wifi`. Test gated like `/anim` because it can move servos. Register in `kRoutes`. Update ApiReference + `docs/api.md` in the same change.

### 4. Wildcard matcher

Simple recursive/iterative glob: only metacharacter `*`, no `?`, no character classes. Match against `type` only. Empty `on` invalid at save.

**Why:** User asked for `*` anywhere; keep firmware tiny.

### 5. Effect runners

- **sound:** reuse `tryStartButtonEventSound` / wav stream helpers (rename if needed so they are Actions-owned, not push-button-owned).
- **anim:** `apiSetAnimation(name)` — unknown name no-ops / returns false; continue other rows.

**Why:** Same paths as existing API/SFX; no extra network stack on device.

### 6. UI

New `ui/src/views/Actions.tsx`, nav **Actions**, route `/actions`. Clear layout:

- List of editable rows (pattern text, kind select, param control, remove)
- Add row
- Save / discard (wifi-style staged draft)
- Separate “Simulate event” strip: text + button → `POST /actions/test`
- Dirty gating: Test/Fire disabled until Save; wildcard rows disable per-row Test

Extensions: remove sound selects; keep event name + hint catalog; point users to Actions for reactions.

Share anim name list with Animations page (extract const).

### 7. Legacy migration

In settings load after Actions blob read: if count==0 and any `btn_snd_*` is bell/click, append sound rows for those five event types, `saveActions`, then delete legacy NVS keys only after a successful write. If Actions non-empty, just delete legacy keys. Remove HTTP fields from GET/POST `/settings` and UI types.

## Risks / Trade-offs

- **[Risk]** Many matching rows start several anims/sounds → Mitigation: anim path already replaces current anim; sound busy rules skip/preempt; document fire-all order.
- **[Risk]** Simulate does not exercise outbound publish → Mitigation: intentional (no host side effects); real button still tests full path.
- **[Risk]** Blob size / max 16 → Mitigation: fixed caps in validate; UI enforces max.
- **[Trade-off]** No host wildcard parity — host rules stay exact-match only until a separate change.

## Migration Plan

1. Ship firmware that loads Actions, migrates legacy sounds, stops exposing `push_button_sound_*`.
2. Users who relied on Extensions sound selects get equivalent Actions rows automatically when Actions was empty.
3. Rollback: older firmware ignores `actions` key and reads legacy sounds again only if those keys still exist — after migration keys are cleared, rollback loses sound mappings unless user reconfigures. Acceptable for this device class; document in release notes.

## Open Questions

None material — caps (16 / 47), simulate-without-publish are recorded above as design defaults.
