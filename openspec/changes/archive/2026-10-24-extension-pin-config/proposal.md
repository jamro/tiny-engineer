# Proposal

## Why

Push-button is fixed to GPIO20 and the Extensions page always shows wiring and event notes, including J4 labels that belong in PCB docs. Users need a selectable Extension-port pin, gated details when the extension is on, and a pin-claim model so future extensions on the same port do not fight over GPIO at runtime. Success toasts that never clear also clutter the UI.

## What Changes

- Add setting `push_button_pin` (default **20**, allowlist **6 / 7 / 20 / 21**). Persist always; turning the extension off does not clear the pin.
- Firmware reads the configured pin when enabled; apply pin/enable changes immediately (re-attach ISR).
- Extensions UI: when the push-button toggle is on (draft), show pin selector, short wiring how-to, and an **API** section (outbound events; no new HTTP routes). When off, only the toggle (and Save). Say **Extension port**, never **J4**, in UI and user-facing docs.
- Introduce a shared extension pin-claim check: only **enabled** extensions claim GPIO; `POST /settings` that would leave two enabled extensions on the same pin is rejected (hard 400, nothing saved). New extensions stay disabled by default so conflicts appear only when enabling or changing pins. Boot: if a corrupt/migrated state has conflicting enabled claims, leave those pins idle and log.
- Web UI: `#status.ok` auto-dismisses after a few seconds; `err` stays until the user dismisses.

## Capabilities

### New Capabilities

- `extension-pin-claims`: Runtime GPIO claims among enabled extensions on the Extension port — allowlist validation, save-time conflict reject, boot idle+log when claims conflict.
- `web-status`: Status bar success messages auto-clear after a short delay; errors remain until dismissed.

### Modified Capabilities

- `push-button`: Configurable pin (`push_button_pin`), gated Extensions UI (config + wire + API), Extension-port wording (no J4 in UI/user docs), idle the configured pin when disabled (not hard-coded GPIO20).

## Impact

- Settings layers: `push_button_pin` / NVS key, validation, GET/POST `/settings`, apply path in `push_button` + settings handlers.
- New small claim helper used at save (and boot apply); push-button registers as the first claim provider.
- UI: `Extensions.tsx`, `api/types.ts`, `ApiReference.tsx`; `StatusProvider` / `StatusBar` for ok auto-dismiss.
- Docs: `docs/api.md`, `docs/settings.md` (and related settings mentions). Hardware pinout may keep J4 as PCB-internal naming.
- Tests: settings validation / pin allowlist; claim conflict; UI status timer if covered by existing UI tests.
