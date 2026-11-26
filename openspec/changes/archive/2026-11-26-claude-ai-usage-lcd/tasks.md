# Tasks

## 1. Shared LCD post

- [x] 1.1 Extract the LCD body (`AI BUDGET:` / right-aligned `NN.NN%`) and the 2000 ms / 1500 ms caps from `packages/tiny-engineer-cli/src/integrations/cursor/lcd-usage.js` into a helper both IDEs import. Verify `npm test --prefix packages/tiny-engineer-cli -- test/lcd-usage.test.js test/usage-remaining.test.js` still passes and the Cursor post body is unchanged.

## 2. Remaining percent

- [x] 2.1 Add a Claude remaining-percent parser that takes the max numeric `utilization` and `limits[].percent`, returns `100 - max` clamped at 0 and rounded to two decimals, and returns null when no number exists. Verify unit tests for weekly 40 / session 20 → `60`, per-model 90 → `10`, utilization ≥ 100 → `0`, and an empty payload → null.

## 3. Login and usage HTTP

- [x] 3.1 Read the bearer read-only: macOS Keychain service `Claude Code-credentials` via `security` with a short timeout, else `~/.claude/.credentials.json` (`claudeAiOauth.accessToken`), accepting a raw token or that JSON blob. Verify tests with fake spawn/file cover keychain hit, file fallback, missing login, and that no test writes credentials or logs the token. If that Keychain service is absent on a logged-in Mac, confirm the real item label without printing the secret and keep only the constant in sync.
- [x] 3.2 Fetch `GET https://api.anthropic.com/api/oauth/usage` with `Authorization: Bearer` and `anthropic-beta: oauth-2025-04-20`. On 403 or a network error, retry once with `curl`, bearer in a stdin config, not argv. Other HTTP failures, missing `curl`, and abort return null and kill the child. Verify tests with a fake `fetch` and a fake spawn: success without curl, 403 then curl success, curl omitted from argv, and abort.

## 4. Hook

- [x] 4.1 Add the fail-open orchestrator and call it from Claude `afterAnim` only when `anim === "ring"` and `TINY_ENGINEER_AI_USAGE_LCD` is `1`. Cap wall time like Cursor, swallow errors, and return before `hardExitAfterPost`. Verify tests: flag unset or `0` skips lookup; non-`ring` skips; happy path posts `AI BUDGET:` and `60.00%` right-aligned; null percent or a thrown post does not throw; the overall timeout returns.

## 5. Copy

- [x] 5.1 Update the setup prompt, CLI help, `docs/hooks.md`, and `docs/integration.md` so the existing flag names Cursor and Claude Code. Adjust `mods/lcd_cabinet/README.md` only if it still says the update is Cursor-only. Verify `npm test --prefix packages/tiny-engineer-cli` passes, including setup tests that match the prompt text.

## 6. Integration check

- [x] 6.1 Run `npm test --prefix packages/tiny-engineer-cli` and confirm the full CLI suite passes.
