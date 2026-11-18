# Design

## Context

See proposal.md — Why. LCD already applies boot content in `lcd1602ApplyFromSettings` before Wi‑Fi; boot then may `setAnimation(Welcome)`. Welcome also runs from `POST /anim`. Text goes through `lcd1602SetText` / `lcd1602ClearText` / GET via `lcd1602TextState()`. Welcome ends via `finishAnimation` from `updateWelcome`. Existing LCD settings use NVS keys `ext_lcd*`.

## Goals / Non-Goals

**Goals:**

- Opt-in overlay tied to every `AnimationId::Welcome` start/end.
- Queue text/clear during overlay; logical GET; no HELLO in `last` NVS.
- Settings + UI + docs/serial parity for the new bool.

**Non-Goals:**

- Custom greeting strings or i18n.
- Overlay on setup-AP `playWelcome()` without welcome anim.
- Deferring brightness/color.
- Changing welcome audio/motion timing.

## Decisions

1. **Setting name**  
   JSON `lcd1602_welcome_hello`, UI **Hello on welcome**, default false. NVS `ext_lcd_hi` (≤15). C++ `lcd1602WelcomeHello` / `settingsLcd1602WelcomeHello()`.

2. **Hook points**  
   - Start overlay when welcome animation actually starts (`startWelcome` / registry start path), if extension on + flag.  
   - End overlay in one place when leaving Welcome — prefer `finishAnimation` when current anim is Welcome, or an explicit `lcd1602OnWelcomeFinished()` called from welcome’s finish path so other one-shots do not touch LCD. Prefer **welcome-specific end hook** from `updateWelcome` before/with `finishAnimation` to avoid coupling every animation finish.  
   - Also clear overlay if Welcome is interrupted by another animation (setAnimation replaces Welcome) — same end path: restore stash or apply queue.

3. **State machine (RAM only)**  
   ```
   idle --> overlay (stash g_text, show HELLO/HUMAN center, no persist)
        --> optional pending logical text (last-wins merges)
        --> idle (apply pending if any else restore stash; persist only on real apply)
   ```
   Visible framebuffer during overlay is HELLO/HUMAN; `g_text` holds logical (stash, then pending merges). GET reads `g_text`. Render path draws overlay strings while `g_overlayActive`.

4. **Text/clear during overlay**  
   Validate as today; merge into `g_text` (logical); set `g_pendingApply=true`; return **200** with logical body; do not `persistLast` until overlay ends and apply runs. DELETE = both lines empty in logical state. Brightness/color still apply and may `persistLast` immediately, but must snapshot **stash** (last-applied) lines — never queued `g_text` — until the pending apply runs.

5. **Boot order**  
   Unchanged: boot content then later Welcome. Overlay stashes whatever boot already set.

6. **Flag false / extension off**  
   No overlay. Mid-overlay disable: end overlay immediately (apply pending or restore) when settings turn extension or flag off. Flag-only off while the extension stays on must **not** re-run `applyBootContent()` (that would wipe logical `g_text`); end overlay + render is enough. Live `lcd1602_backlight_sleep` / `lcd1602_welcome_hello` changes must also keep runtime text (and any pending apply) intact — awake brightness force-on only if follow-sleep turns off and brightness is 0; ending the overlay then persists the accepted text, including when boot is `last`. Full boot reapply still runs for enable, boot/line changes, and boot/`changed == nullptr`.

## Risks / Trade-offs

- [Welcome interrupted by another anim] → Must end overlay on leave-Welcome, not only natural finish. Mitigate: end hook from animation controller when leaving Welcome.  
- [Agent GET expects visible text] → Spec says logical; document in api.md.  
- [NVS key budget] → `ext_lcd_hi` fits.

## Migration Plan

- New key absent → false. No migration. Rollback: ignore unknown JSON field.

## Open Questions

None — every-welcome trigger and logical GET/pending confirmed in explore.
