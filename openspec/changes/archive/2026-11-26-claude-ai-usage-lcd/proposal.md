# Proposal

## Why

The LCD cabinet already shows remaining Cursor plan budget after a finished turn when `TINY_ENGINEER_AI_USAGE_LCD=1`. Claude Code turns end on the same robot and leave that line stale. The same opt-in should refresh the display from Claude's plan windows.

## What Changes

- After a completed Claude Code turn (`Stop` mapped to `ring`), and only when `TINY_ENGINEER_AI_USAGE_LCD=1`, write LCD line 1 `CLAUDE BUDGET` and line 2 one remaining percent (`NN.NN%`, right-aligned). A Cursor turn writes line 1 `CURSOR BUDGET` with the same line 2.
- Remaining percent is `100 - max(utilization)` across every usage window in the Claude OAuth usage payload (5-hour session, weekly, and any per-model window present), clamped at 0, two decimal places.
- Read the Claude Code OAuth token only (macOS Keychain, else `~/.claude/.credentials.json`). Do not refresh or write credentials.
- Fetch `GET https://api.anthropic.com/api/oauth/usage`. If Node `fetch` is rejected, retry once with the system `curl`.
- Fail-open: missing login, API-key login, expired token, slow or failed HTTP, disabled LCD, or robot errors stay silent and do not change the hook exit code. Wall time stays capped like the Cursor path.
- Cursor budget math, session lookup, and LCD body stay as they are. Whichever IDE finishes a turn last overwrites the two lines.
- Setup prompt, CLI help, and hook docs name both Cursor and Claude Code under the existing flag. No second env key.

## Capabilities

### New Capabilities

- `cli-ai-usage-lcd`: Opt-in remaining-plan readout on LCD1602 after a completed agent turn. This change adds the Claude Code source and the one-number rule; the Cursor source stays.

### Modified Capabilities

None. The flag is not specified under `cli-user-config` or `lcd1602`. Firmware text routes do not change.

## Impact

- Package: `packages/tiny-engineer-cli` (`integrations/claude-code`, shared or sibling usage helpers next to `integrations/cursor/lcd-usage.js`, setup copy, tests).
- Docs: `docs/hooks.md`, `docs/integration.md`, CLI help. `mods/lcd_cabinet/README.md` only if the one-line "any agent" note needs the Claude trigger.
- No firmware, HTTP route, or NVS change. No new npm dependency.
