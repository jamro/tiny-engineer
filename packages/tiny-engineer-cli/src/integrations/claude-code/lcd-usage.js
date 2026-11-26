import {
  AI_USAGE_LCD_KEY,
  CLAUDE_BUDGET_LABEL,
  OVERALL_TIMEOUT_MS,
  STEP_TIMEOUT_MS,
  postBudgetLcd,
} from "../ai-usage-lcd.js";
import { fetchClaudeUsage } from "./usage-api.js";
import { remainingBudgetPercent } from "./usage-remaining.js";
import { resolveClaudeAccessToken } from "./usage-session.js";

export { AI_USAGE_LCD_KEY };

/**
 * @typedef {{
 *   url?: string,
 *   transport?: string,
 *   port?: string | null,
 * }} TransportOpts
 */

/**
 * @typedef {{
 *   resolveToken?: (opts?: { signal?: AbortSignal }) => Promise<string | null>,
 *   fetchUsage?: (token: string, opts?: { timeoutMs?: number, signal?: AbortSignal }) => Promise<Record<string, unknown> | null>,
 *   postLcd?: (pct: number, transport: TransportOpts) => Promise<unknown>,
 *   overallTimeoutMs?: number,
 *   stepTimeoutMs?: number,
 *   env?: NodeJS.ProcessEnv,
 * }} LcdUsageDeps
 */

/**
 * After a completed Claude Code turn (`ring`), optionally show remaining plan budget.
 * Fail-open: never throws; capped wall time; silent on any error.
 *
 * @param {TransportOpts} [transport]
 * @param {LcdUsageDeps} [deps]
 * @returns {Promise<void>}
 */
export async function maybeShowClaudeUsageOnLcd(transport = {}, deps = {}) {
  const env = deps.env ?? process.env;
  if (env[AI_USAGE_LCD_KEY] !== "1") return;

  const overallTimeoutMs = deps.overallTimeoutMs ?? OVERALL_TIMEOUT_MS;
  const stepTimeoutMs = deps.stepTimeoutMs ?? STEP_TIMEOUT_MS;
  const resolveToken = deps.resolveToken ?? resolveClaudeAccessToken;
  const fetchUsage = deps.fetchUsage ?? fetchClaudeUsage;
  const postLcd = deps.postLcd ?? ((pct, opts) => postBudgetLcd(pct, opts, stepTimeoutMs, CLAUDE_BUDGET_LABEL));

  const ac = new AbortController();
  let cancelled = false;
  const run = (async () => {
    const token = await resolveToken({ signal: ac.signal });
    if (cancelled || ac.signal.aborted || !token) return;

    const usage = await fetchUsage(token, { timeoutMs: stepTimeoutMs, signal: ac.signal });
    if (cancelled || ac.signal.aborted || !usage) return;

    const pct = remainingBudgetPercent(usage);
    if (cancelled || ac.signal.aborted || pct == null) return;

    await postLcd(pct, transport);
  })();

  /** @type {ReturnType<typeof setTimeout> | undefined} */
  let overallTimer;
  const overallDeadline = new Promise((resolve) => {
    overallTimer = setTimeout(resolve, overallTimeoutMs);
  });

  try {
    await Promise.race([run.catch(() => {}), overallDeadline]);
  } catch {
    // fail-open
  } finally {
    cancelled = true;
    ac.abort();
    if (overallTimer !== undefined) clearTimeout(overallTimer);
  }
}
