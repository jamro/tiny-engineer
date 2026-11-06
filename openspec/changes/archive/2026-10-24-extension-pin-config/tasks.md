# Tasks

## 1. Push-button pin setting

- [x] 1.1 Add `push_button_pin` (NVS `ext_btn_pin`, default 20, allowlist 6/7/20/21) through the settings layers per `docs/settings.md`, expose on `GET`/`POST /settings`, and verify `pio test -e native` passes
- [x] 1.2 Document `push_button_pin` in `docs/api.md`, `docs/settings.md`, and any `GET /settings` field lists that already mention `push_button`, using Extension port (not J4), and verify each lists default 20, allowlist, persist-when-off, and immediate apply

## 2. Pin claims

- [x] 2.1 Add an extension pin-claim helper that collects claims from registered providers on proposed settings, rejects duplicate GPIOs with a plain 400 naming pin and extension ids, register push-button as the first provider, call it from the settings save path before write, and verify native tests cover allowlist reject, same-pin two-enabled reject, and disabled-same-pin allow (`pio test -e native`)
- [x] 2.2 On boot/apply, if loaded claims conflict, leave conflicting pins idle, keep NVS unchanged, log the conflict, still apply non-conflicting enables, and verify with a native or documented boot-path test/assertion that conflicting providers do not attach

## 3. Push-button GPIO bind

- [x] 3.1 Change `pushButtonApply` to take the configured pin (detach previous, idle when off, INPUT_PULLUP + ISR when on), wire callers from boot and settings/serial apply using `settingsPushButton()` + `settingsPushButtonPin()`, stop treating `PUSH_BUTTON_PIN` as the only runtime pin, and verify `pio run` compiles
- [x] 3.2 Update the GPIO20 / extension-port note in `docs/hardware/pinout.md` so push-button uses the configured Extension-port pin (default GP20) without requiring UI/user docs to say J4, and verify the note matches the pin setting

## 4. Extensions UI

- [x] 4.1 Extend `Extensions.tsx` so draft toggle off shows only the toggle (+ Save); draft on shows pin select (GP6/7/20/21), Extension-port wiring how-to, and API section (button events, no extra HTTP); post `push_button` + `push_button_pin`; update `ui/src/api/types.ts` and `ApiReference.tsx`; remove J4 from UI copy; verify `npm run lint --prefix ui` and `npm test --prefix ui` pass

## 5. Status ok auto-dismiss

- [x] 5.1 Auto-clear `#status.ok` after 3 seconds in the status provider (cancel on newer status / clear / unmount); leave `err` until dismiss; verify with a UI unit test or manual check that ok clears and err remains (`npm test --prefix ui` if a test is added)

## 6. Integration

- [x] 6.1 Run `pio run` after firmware groups and verify the default env links

## Workflow follow-up

- Archive the change after review (`/opsx-archive` or project equivalent).
- Sync delta specs into main specs on archive.
