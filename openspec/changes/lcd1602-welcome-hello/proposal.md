# Proposal

## Why

Welcome already says “Hello, human” in audio and motion, but the optional LCD1602 stays on boot/agent text through that moment. A small opt-in overlay makes the desk display match the greeting without replacing normal LCD content afterward.

## What Changes

- Add boolean setting **`lcd1602_welcome_hello`** (default **false**), persisted when the extension is off, apply immediately; UI label **Hello on welcome** on Extensions.
- When the extension is on and the flag is true, every `welcome` animation (boot after Wi‑Fi and `POST /anim?name=welcome`) overlays centered **HELLO** / **HUMAN** on the LCD for the duration of that animation.
- When welcome ends, restore the pre-overlay logical text (or apply any text/clear updates queued during the overlay).
- Text/clear HTTP and serial `ext` updates during the overlay are accepted (**200**), last-wins queued, and do not change the visible HELLO lines until welcome finishes.
- `GET` text during overlay returns **logical** content: pending queued text if any, otherwise the stashed pre-overlay state — never the HELLO overlay itself.
- Do not persist HELLO/HUMAN as `last` state.
- Out of scope: setup-AP `playWelcome()` without the welcome animation; delaying brightness/color; custom hello strings.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `lcd1602`: Welcome-hello setting, overlay behavior, queued text/clear, logical GET during overlay.
- `serial-protocol`: Serial settings LCD slice includes `lcd1602_welcome_hello`.

## Impact

- Firmware: settings checklist + NVS key; LCD overlay/queue state; hooks on welcome start/finish; text/clear/GET paths.
- HTTP/serial settings JSON; Extensions UI + ApiReference; docs (`api.md`, `settings.md`, `serial.md`, testing summaries as needed).
- Non-breaking (new optional field, default off).
