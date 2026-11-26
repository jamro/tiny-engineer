/**
 * @param {unknown} value
 * @param {number[]} out
 */
function pushFinite(value, out) {
  if (value == null || value === "") return;
  const n = Number(value);
  if (Number.isFinite(n)) out.push(n);
}

/**
 * Utilization percents only. Skips credit and dollar amounts.
 * @param {unknown} node
 * @param {number[]} out
 * @param {number} depth
 */
function collectUtilization(node, out, depth) {
  if (!node || typeof node !== "object" || depth > 4) return;
  if (Array.isArray(node)) {
    for (const entry of node) collectUtilization(entry, out, depth + 1);
    return;
  }
  const rec = /** @type {Record<string, unknown>} */ (node);
  pushFinite(rec.utilization, out);
  for (const value of Object.values(rec)) {
    if (value && typeof value === "object") collectUtilization(value, out, depth + 1);
  }
}

/**
 * Remaining plan percent: 100 - max(utilization), clamped at 0, two decimals.
 * Reads every window `utilization` and `limits[].percent`.
 * `spend.percent` is only a fallback: the API already rounds it (19.77 → 20).
 *
 * @param {unknown} payload
 * @returns {number | null}
 */
export function remainingBudgetPercent(payload) {
  if (!payload || typeof payload !== "object") return null;
  const root = /** @type {Record<string, unknown>} */ (payload);
  /** @type {number[]} */
  const used = [];

  collectUtilization(root, used, 0);

  if (Array.isArray(root.limits)) {
    for (const entry of root.limits) {
      if (!entry || typeof entry !== "object") continue;
      pushFinite(/** @type {Record<string, unknown>} */ (entry).percent, used);
    }
  }

  if (!used.length) {
    const spend = root.spend;
    if (spend && typeof spend === "object" && !Array.isArray(spend)) {
      pushFinite(/** @type {Record<string, unknown>} */ (spend).percent, used);
    }
  }

  if (!used.length) return null;
  return Math.round(Math.max(100 - Math.max(...used), 0) * 100) / 100;
}
