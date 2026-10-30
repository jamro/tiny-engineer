# Design

## Context

See proposal.md — Why. Today `push_button` is a bool; GPIO is hard-coded `PUSH_BUTTON_PIN = 20` in `pins.h` / `push_button.cpp`. Extensions UI always shows wiring + events and mentions J4. Settings validation is per-field (`invalid <field>`); no cross-extension pin awareness. Status `ok` stays until ×.

## Goals / Non-Goals

**Goals:**
- Configurable push-button pin with allowlist and immediate apply.
- Gated Extensions UI (draft toggle) with Extension-port wording.
- Extensible pin-claim check at save; boot idle+log on conflict.
- `#status.ok` auto-dismiss ~3s.

**Non-Goals:**
- Second hardware extension in this change (registry must accept a second provider later with little churn).
- Structured JSON error bodies for conflicts (plain 400 string).
- Auto-disable of conflicting extensions on save or boot.
- Changing gesture timings or outbound event types.
- Renaming J4 in PCB/hardware pinout docs.

## Decisions

### 1. Setting shape: `push_button_pin` integer

- JSON/HTTP: `push_button_pin` (number). NVS: `ext_btn_pin` (≤15). Default 20. Allowlist `{6,7,20,21}`.
- Persist always; off does not clear.
- Apply: `pushButtonApply(enabled, pin)` detach old / attach new / idle configured pin when off.
- Alternatives: string `"GP20"` — rejected (worse validation, more surface). Nested `extensions.push_button.pin` — rejected (settings stay flat like today).

### 2. Claim registry at save and boot

```
proposed = merge(current, patch)
claims = collect from each registered provider(proposed)
if duplicate GPIO among claims -> 400, no write
else write + apply
```

- Provider API (sketch): given proposed settings values, return `{id, pins[]}` or empty when disabled.
- Push-button is the only provider now; claims empty or `{push-button, [pin]}`.
- Conflict message: `pin conflict on GP20 (push-button, …)` — keep `sendError` plain string.
- Per-extension allowlist stays in that extension’s field validate (`invalid push_button_pin`) before claim step.
- Boot: after load, if claims conflict → do not attach conflicting providers; log; leave NVS as-is. User fixes via POST that resolves (disable one or change pin). Unrelated settings POST while conflict remains also 400 until resolved (forces cleanup; disable/change-pin posts that clear the conflict still succeed).

Alternatives: soft warn — rejected. Auto-disable other — rejected. Check only fields in patch — rejected (can leave inconsistent proposed state).

### 3. Extensions UI gating on draft

- Reveal pin / wire / API when `draft.pushButton === true`, not only after save.
- Single Save posts both `push_button` and `push_button_pin` when relevant (always send pin when on; when off, still OK to send last pin or omit — prefer include current draft pin so off+pin edits if we ever show pin while off are not needed; while off pin control hidden, send `push_button: false` only or both with last pin — send both for simpler draft sync).
- Labels: GP6 / GP7 / GP20 / GP21; copy says Extension port; no J4.
- API section: list `button.down|up|click|double_click|long_click`, sinks WS + serial `Evt`, note no extra HTTP endpoints.

### 4. Status ok timer in StatusProvider

- On `setStatus(..., "ok")`, start 3000 ms timer → `clearStatus` if message generation still matches.
- Clear/replace timer on every `setStatus` / `clearStatus` / unmount.
- `err` and `loading`: no auto-clear.
- Constant `STATUS_OK_MS = 3000` next to status module.

Alternative: only StatusBar useEffect — also fine; prefer provider so all `done:` paths covered without per-view logic.

### 5. Docs wording

- `docs/api.md`, `docs/settings.md`, UI, ApiReference: Extension port + configurable pin.
- `docs/hardware/pinout.md` may keep J4 (PCB-internal).

## Risks / Trade-offs

- [Corrupt dual-enable blocks unrelated settings saves] → Mitigation: error names pin + extensions; fix is one POST disable or retarget pin. Rare if every save validates.
- [GP20/GP21 also UART0 silkscreen] → Mitigation: wiring hint notes Extension port GND; console stays USB CDC (existing hardware docs).
- [ISR pin switch mid-press] → Mitigation: clear queue + `logic.apply` sample on rebind (same as enable path today).
- [Future multi-pin extension] → Mitigation: claims use `pins[]` from day one.

## Migration Plan

- Missing NVS `ext_btn_pin` → default 20 (same as old hard-code).
- No firmware migration of enabled state.
- Rollback: older firmware ignores unknown `push_button_pin` field on POST if clients send it — actually old firmware returns `unknown field`. Clients/UI in same release must ship together. GET without the field on old FW: new UI should tolerate missing pin (default 20 client-side) only if mixed versions matter; prefer same release flash + UI embed via `pio run`.

## Open Questions

None material — explore locked persist-always, hard reject, boot idle+log, 3s ok dismiss.
