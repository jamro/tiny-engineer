import { robotExt } from "../robot/ops.js";

/** Opt-in: only `"1"` enables LCD AI budget updates after a completed turn. */
export const AI_USAGE_LCD_KEY = "TINY_ENGINEER_AI_USAGE_LCD";

export const OVERALL_TIMEOUT_MS = 2000;
export const STEP_TIMEOUT_MS = 1500;

/** LCD1602 line 1. 13 characters, fits the 16-column display. */
export const CURSOR_BUDGET_LABEL = "CURSOR BUDGET";
export const CLAUDE_BUDGET_LABEL = "CLAUDE BUDGET";

/**
 * @typedef {{
 *   url?: string,
 *   transport?: string,
 *   port?: string | null,
 * }} BudgetTransportOpts
 */

/**
 * Shared LCD1602 body for remaining plan percent.
 * @param {number} pct
 * @param {string} label line 1, `CURSOR BUDGET` or `CLAUDE BUDGET`
 */
export function budgetLcdBody(pct, label) {
  return {
    line1: { text: label },
    line2: { text: `${pct.toFixed(2)}%`, align: "right" },
  };
}

/**
 * @param {number} pct
 * @param {BudgetTransportOpts} transport
 * @param {number} timeoutMs
 * @param {string} label
 */
export async function postBudgetLcd(pct, transport, timeoutMs, label) {
  await robotExt(
    {
      id: "lcd1602",
      suffix: "text",
      method: "POST",
      body: budgetLcdBody(pct, label),
    },
    { ...transport, timeoutMs },
  );
}
