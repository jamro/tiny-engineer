# Design

## Context

Cursor already posts LCD budget text from `integrations/cursor/index.js` `afterAnim` when the animation is `ring` and `TINY_ENGINEER_AI_USAGE_LCD=1`. The poster, 2 s overall cap, and 1.5 s step cap live in `integrations/cursor/lcd-usage.js`. Claude Code maps `Stop` to `ring` and sets `hardExitAfterPost`, so `cli.js` awaits `afterAnim` and then exits. See proposal.md for why Claude is missing.

Claude subscription windows are not in `~/.claude/stats-cache.json`. They come from the signed-in Claude Code OAuth session: macOS Keychain item `Claude Code-credentials`, otherwise `~/.claude/.credentials.json` (`claudeAiOauth.accessToken`). Usage is `GET https://api.anthropic.com/api/oauth/usage` with `anthropic-beta: oauth-2025-04-20`. The JSON carries named windows (`utilization`), a `limits` array (`percent` used), or, on enterprise logins with null plan windows, `extra_usage.utilization` (`spend.percent` is that same figure already rounded). Node `fetch` is sometimes rejected with 403; `curl` is accepted.

## Goals / Non-Goals

**Goals:**

- One Claude `afterAnim` path that posts the same two LCD lines as Cursor.
- One remaining percent from every numeric window in whichever response shape arrives.
- Read-only credentials, fail-open, same time cap, no new dependency.

**Non-Goals:**

- Refreshing or writing OAuth tokens.
- A second env flag, dollar `extra_usage`, or `stats-cache.json` as the source.
- Changing Cursor's formula or firmware LCD routes.
- An env override token for Claude (tests inject fakes).

## Decisions

### Hook point stays `afterAnim` on `ring`

Claude `index.js` gains `afterAnim`. It returns immediately unless `ctx.anim === "ring"`, then awaits the usage helper. `cli.js` already awaits that hook before `process.exit` on `hardExitAfterPost`, so the helper must finish or abort inside the cap and must not leave a `curl` child running.

Alternative: a separate Claude hook script. Rejected. Setup already installs `hook claude-code`, and `Stop` already maps to `ring`.

### Sibling modules, shared LCD body

New files under `integrations/claude-code/`: session read, usage HTTP, remaining-percent parse, and the fail-open orchestrator. The orchestrator posts through the existing `robotExt` body (line 1 `CLAUDE BUDGET` or `CURSOR BUDGET`, line 2 `{ text: percent, align: "right" }`). Extract that body plus the 2000/1500 ms caps into one small helper both IDEs call. Each IDE passes its own line 1. Cursor math and session code stay put.

Alternative: copy the POST into the Claude file only. Rejected. The spec requires the same lines, and two copies will diverge.

### Parse every numeric window, then max

Collect `utilization` from every window object in the payload, including enterprise `extra_usage.utilization`, plus `percent` from each `limits` entry. Use `spend.percent` only when no utilization exists: that field is already rounded (19.77 shows up as 20). Ignore nulls, non-finite numbers, and raw credit or dollar amounts. Remaining is `Math.round(Math.max(100 - Math.max(...used), 0) * 100) / 100`, same rounding as Cursor. Empty set returns null and skips the LCD post. Inactive session windows still count when they carry a number; a 0 does not hide a fuller weekly window.

### Credentials are read-only

On macOS, read Keychain `Claude Code-credentials` with `security find-generic-password -s … -w`, timeout like the Cursor keychain read. If that yields nothing, read `~/.claude/.credentials.json`. Accept either a raw access token or a JSON blob whose `claudeAiOauth.accessToken` is the bearer. Never log the token. Never call the refresh endpoint and never write the file or Keychain.

Alternative: refresh on expiry. Rejected. A hook must not mutate the login Claude Code owns.

### `fetch`, then one `curl`

Call the usage URL with `fetch` first. On 403 or a network error, retry once with `curl`, passing the bearer via a stdin config so it is not on the argv. Any other HTTP error, missing `curl`, or abort returns null. Both calls sit inside the step timeout and the overall abort.

Alternative: always `curl`. Rejected. `fetch` works when the edge allows it and avoids a process spawn.

### Copy, not a new flag

Setup prompt, CLI help, and `docs/hooks.md` / `docs/integration.md` say the existing flag covers Cursor and Claude Code. The prompt is shown only when at least one selected IDE supports the readout. Antigravity never sees the question, and `--ai-usage-lcd` / `--no-ai-usage-lcd` are ignored for it. Enabling prints that IDE's sign-in steps: Cursor uses the local account, Claude Code needs `/login`. `mods/lcd_cabinet/README.md` stays generic unless a sentence still says the update is Cursor-only.

## Risks / Trade-offs

- [Undocumented usage JSON changes] → Collect both shapes; no numeric window means skip the post, exit 0.
- [Node `fetch` gets 403] → One `curl` retry; still inside the cap.
- [Keychain dialog or hang blocks the hook] → Short `security` timeout, then file fallback, then skip.
- [Token leaks into `ps` or logs] → `curl` config on stdin; no token in logs or error strings.
- [Cursor and Claude overwrite one LCD] → Accepted. Later `ring` wins. Same label.
- [API-key login has no windows] → Null percent, no post.

## Migration Plan

No data migration. The flag default stays off. Rollback is reverting the CLI. Existing Cursor setups keep working because their lookup is unchanged.

## Open Questions

None. Keychain item name is `Claude Code-credentials`; if a logged-in Mac has no such item, apply checks the item label without printing the secret and updates only that constant.
