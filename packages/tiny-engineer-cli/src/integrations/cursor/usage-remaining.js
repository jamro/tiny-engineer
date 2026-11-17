/**
 * Remaining plan budget percent from Cursor cycle-limits JSON.
 * Mirrors cursor-usage-tiny-engineer: 100 - max(autoPercentUsed, apiPercentUsed),
 * clamped to ≥0, rounded to two decimal places.
 *
 * @param {unknown} payload
 * @returns {number | null}
 */
export function remainingBudgetPercent(payload) {
  if (!payload || typeof payload !== "object") return null;

  const root = /** @type {Record<string, unknown>} */ (payload);
  const individual =
    root.individualUsage && typeof root.individualUsage === "object"
      ? /** @type {Record<string, unknown>} */ (root.individualUsage)
      : null;
  const planRaw =
    (root.planUsage && typeof root.planUsage === "object" ? root.planUsage : null) ??
    (individual?.plan && typeof individual.plan === "object" ? individual.plan : null) ??
    {};
  const plan = /** @type {Record<string, unknown>} */ (planRaw);

  const used = [plan.autoPercentUsed, plan.apiPercentUsed]
    .filter((v) => v != null && v !== "")
    .map((v) => Number(v))
    .filter((n) => Number.isFinite(n));

  if (!used.length) return null;

  return Math.round(Math.max(100 - Math.max(...used), 0) * 100) / 100;
}
