import {
  AI_USAGE_LCD_KEY,
  CURSOR_BUDGET_LABEL,
  OVERALL_TIMEOUT_MS,
  STEP_TIMEOUT_MS,
  postBudgetLcd,
} from "../ai-usage-lcd.js";
import { fetchCursorLimits } from "./usage-api.js";
import { remainingBudgetPercent } from "./usage-remaining.js";
import { resolveCursorSessionCookie } from "./usage-session.js";

export { AI_USAGE_LCD_KEY, OVERALL_TIMEOUT_MS, STEP_TIMEOUT_MS };

/**
 * @typedef {{
 *   url?: string,
 *   transport?: string,
 *   port?: string | null,
 * }} TransportOpts
 */

/**
 * @typedef {{
 *   resolveCookie?: (opts?: { signal?: AbortSignal }) => Promise<string | null>,
 *   fetchLimits?: (cookie: string, opts?: { timeoutMs?: number }) => Promise<Record<string, unknown> | null>,
 *   postLcd?: (pct: number, transport: TransportOpts) => Promise<unknown>,
 *   overallTimeoutMs?: number,
 *   stepTimeoutMs?: number,
 *   env?: NodeJS.ProcessEnv,
 * }} LcdUsageDeps
 */

/**
 * @param {number} pct
 * @param {TransportOpts} transport
 * @param {number} timeoutMs
 */
function defaultPostLcd(pct, transport, timeoutMs) {
  return postBudgetLcd(pct, transport, timeoutMs, CURSOR_BUDGET_LABEL);
}

/**
 * After a completed Cursor turn (`ring`), optionally show remaining AI budget on LCD1602.
 * Fail-open: never throws; capped wall time; silent on any error.
 *
 * @param {TransportOpts} [transport]
 * @param {LcdUsageDeps} [deps]
 * @returns {Promise<void>}
 */
export async function maybeShowCursorUsageOnLcd(transport = {}, deps = {}) {
  const env = deps.env ?? process.env;
  if (env[AI_USAGE_LCD_KEY] !== "1") return;

  const overallTimeoutMs = deps.overallTimeoutMs ?? OVERALL_TIMEOUT_MS;
  const stepTimeoutMs = deps.stepTimeoutMs ?? STEP_TIMEOUT_MS;
  const resolveCookie = deps.resolveCookie ?? resolveCursorSessionCookie;
  const fetchLimits = deps.fetchLimits ?? fetchCursorLimits;
  const postLcd =
    deps.postLcd ??
    ((pct, opts) => defaultPostLcd(pct, opts, stepTimeoutMs));

  const ac = new AbortController();
  let cancelled = false;
  const run = (async () => {
    const cookie = await resolveCookie({ signal: ac.signal });
    if (cancelled || ac.signal.aborted || !cookie) return;

    const limits = await fetchLimits(cookie, { timeoutMs: stepTimeoutMs });
    if (cancelled || ac.signal.aborted || !limits) return;

    const pct = remainingBudgetPercent(limits);
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
