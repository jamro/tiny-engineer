import { robotExt } from "../../robot/ops.js";
import { fetchCursorLimits } from "./usage-api.js";
import { remainingBudgetPercent } from "./usage-remaining.js";
import { resolveCursorSessionCookie } from "./usage-session.js";

/** Opt-in: only `"1"` enables LCD AI budget updates after Cursor `ring`. */
export const AI_USAGE_LCD_KEY = "TINY_ENGINEER_AI_USAGE_LCD";

export const OVERALL_TIMEOUT_MS = 2000;
export const STEP_TIMEOUT_MS = 1500;

/**
 * @param {number} ms
 * @returns {Promise<void>}
 */
function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

/**
 * @typedef {{
 *   url?: string,
 *   transport?: string,
 *   port?: string | null,
 * }} TransportOpts
 */

/**
 * @typedef {{
 *   resolveCookie?: () => Promise<string | null>,
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
async function defaultPostLcd(pct, transport, timeoutMs) {
  await robotExt(
    {
      id: "lcd1602",
      suffix: "text",
      method: "POST",
      body: {
        line1: { text: "AI BUDGET:" },
        line2: { text: `${pct.toFixed(2)}%`, align: "right" },
      },
    },
    { ...transport, timeoutMs },
  );
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

  let cancelled = false;
  const run = (async () => {
    const cookie = await resolveCookie();
    if (cancelled || !cookie) return;

    const limits = await fetchLimits(cookie, { timeoutMs: stepTimeoutMs });
    if (cancelled || !limits) return;

    const pct = remainingBudgetPercent(limits);
    if (cancelled || pct == null) return;

    await postLcd(pct, transport);
  })();

  try {
    await Promise.race([run.catch(() => {}), sleep(overallTimeoutMs)]);
  } catch {
    // fail-open
  } finally {
    cancelled = true;
  }
}
